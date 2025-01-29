// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "numpy.h"

namespace codon {
namespace ir {
namespace transform {
namespace numpy {
namespace {
using CFG = analyze::dataflow::CFGraph;
using CFBlock = analyze::dataflow::CFBlock;
using RD = analyze::dataflow::RDInspector;
using SE = analyze::module::SideEffectResult;

struct GetVars : public util::Operator {
  std::unordered_set<id_t> &vids;

  explicit GetVars(std::unordered_set<id_t> &vids) : util::Operator(), vids(vids) {}

  void preHook(Node *v) override {
    for (auto *var : v->getUsedVariables()) {
      if (!isA<Func>(var))
        vids.insert(var->getId());
    }
  }
};

struct OkToForwardPast : public util::Operator {
  std::unordered_set<id_t> &vids;
  const std::unordered_map<id_t, NumPyExpr *> &parsedValues;
  SE *se;
  bool ok;

  OkToForwardPast(std::unordered_set<id_t> &vids,
                  const std::unordered_map<id_t, NumPyExpr *> &parsedValues, SE *se)
      : util::Operator(), vids(vids), parsedValues(parsedValues), se(se), ok(true) {}

  void preHook(Node *v) override {
    if (!ok) {
      return;
    } else if (auto *assign = cast<AssignInstr>(v)) {
      if (vids.count(assign->getLhs()->getId()))
        ok = false;
    } else if (auto *val = cast<Value>(v)) {
      auto it = parsedValues.find(val->getId());
      if (it != parsedValues.end()) {
        it->second->apply([&](NumPyExpr &e) {
          if (e.isLeaf() && se->hasSideEffect(e.val))
            ok = false;
        });
        // Skip children since we are processing them manually above.
        for (auto *used : val->getUsedValues())
          see(used);
      } else if (se->hasSideEffect(val)) {
        ok = false;
      }
    }
  }
};

struct GetAllUses : public util::Operator {
  Var *var;
  std::vector<Value *> &uses;

  GetAllUses(Var *var, std::vector<Value *> &uses)
      : util::Operator(), var(var), uses(uses) {}

  void preHook(Node *n) override {
    if (auto *v = cast<Value>(n)) {
      auto vars = v->getUsedVariables();
      if (std::find(vars.begin(), vars.end(), var) != vars.end())
        uses.push_back(v);
    }
  }
};

bool canForwardExpressionAlongPath(
    Value *source, Value *destination, std::unordered_set<id_t> &vids,
    const std::unordered_map<id_t, NumPyExpr *> &parsedValues, SE *se,
    const std::vector<CFBlock *> &path) {
  if (path.empty())
    return true;

  bool go = false;
  for (auto *block : path) {
    for (const auto *value : *block) {
      // Skip things before 'source' in first block
      if (!go && block == path.front() && value == source) {
        go = true;
        continue;
      }

      // Skip things after 'destination' in last block
      if (go && block == path.back() && value == destination) {
        go = false;
        break;
      }

      if (!go)
        continue;

      OkToForwardPast check(vids, parsedValues, se);
      const_cast<Value *>(value)->accept(check);
      if (!check.ok)
        return false;
    }
  }
  return true;
}

bool canForwardExpression(NumPyOptimizationUnit *expr, Value *target,
                          const std::unordered_map<id_t, NumPyExpr *> &parsedValues,
                          CFG *cfg, SE *se) {
  std::unordered_set<id_t> vids;
  bool pure = true;

  expr->expr->apply([&](NumPyExpr &e) {
    if (e.isLeaf()) {
      if (se->hasSideEffect(e.val)) {
        pure = false;
      } else {
        GetVars gv(vids);
        e.val->accept(gv);
      }
    }
  });

  if (!pure)
    return false;

  auto *source = expr->assign;
  auto *start = cfg->getBlock(source);
  auto *end = cfg->getBlock(target);
  seqassertn(start, "start CFG block not found");
  seqassertn(end, "end CFG block not found");
  bool ok = true;

  std::function<void(CFBlock *, std::vector<CFBlock *> &)> dfs =
      [&](CFBlock *curr, std::vector<CFBlock *> &path) {
        path.push_back(curr);
        if (curr == end) {
          if (!canForwardExpressionAlongPath(source, target, vids, parsedValues, se,
                                             path))
            ok = false;
        } else {
          for (auto it = curr->successors_begin(); it != curr->successors_end(); ++it) {
            if (std::find(path.begin(), path.end(), *it) != path.end())
              dfs(*it, path);
          }
        }
        path.pop_back();
      };

  std::vector<CFBlock *> path;
  dfs(start, path);
  return ok;
}

bool canForwardVariable(AssignInstr *assign, Value *destination, BodiedFunc *func,
                        RD *rd) {
  auto *var = assign->getLhs();

  // Check 1: Only the given assignment should reach the destination.
  auto reaching = rd->getReachingDefinitions(var, destination);
  if (reaching.size() != 1 && *reaching.begin() != assign->getRhs()->getId())
    return false;

  // Check 2: There should be no other uses of the variable that the given assignment
  // reaches.
  std::vector<Value *> uses;
  GetAllUses gu(var, uses);
  func->accept(gu);
  for (auto *use : uses) {
    if (use != destination && use->getId() != assign->getId() &&
        rd->getReachingDefinitions(var, use).count(assign->getRhs()->getId()))
      return false;
  }

  return true;
}

ForwardingDAG buildForwardingDAG(BodiedFunc *func, RD *rd, CFG *cfg, SE *se,
                                 std::vector<NumPyOptimizationUnit> &exprs) {
  std::unordered_map<id_t, NumPyExpr *> parsedValues;
  for (auto &e : exprs) {
    e.expr->apply([&](NumPyExpr &e) {
      if (e.val)
        parsedValues.emplace(e.val->getId(), &e);
    });
  }

  ForwardingDAG dag;
  int64_t dstId = 0;
  for (auto &dst : exprs) {
    auto *target = dst.expr.get();
    auto &forwardingVec = dag[&dst];

    std::vector<std::pair<Var *, NumPyExpr *>> vars;
    target->apply([&](NumPyExpr &e) {
      if (e.isLeaf()) {
        if (auto *v = cast<VarValue>(e.val)) {
          vars.emplace_back(v->getVar(), &e);
        }
      }
    });

    for (auto &p : vars) {
      int64_t srcId = 0;
      for (auto &src : exprs) {
        if (srcId != dstId && src.assign && src.assign->getLhs() == p.first) {
          auto checkFwdVar = canForwardVariable(src.assign, p.second->val, func, rd);
          auto checkFwdExpr =
              canForwardExpression(&src, p.second->val, parsedValues, cfg, se);
          if (checkFwdVar && checkFwdExpr)
            forwardingVec.push_back({&dst, &src, p.first, p.second, dstId, srcId});
        }
        ++srcId;
      }
    }
    ++dstId;
  }

  return dag;
}

struct UnionFind {
  std::vector<int64_t> parent;
  std::vector<int64_t> rank;

  explicit UnionFind(int64_t n) : parent(n), rank(n) {
    for (auto i = 0; i < n; i++) {
      parent[i] = i;
      rank[i] = 0;
    }
  }

  int64_t find(int64_t u) {
    if (parent[u] != u)
      parent[u] = find(parent[u]);
    return parent[u];
  }

  void union_(int64_t u, int64_t v) {
    auto ru = find(u);
    auto rv = find(v);
    if (ru != rv) {
      if (rank[ru] > rank[rv]) {
        parent[rv] = ru;
      } else if (rank[ru] < rank[rv]) {
        parent[ru] = rv;
      } else {
        parent[rv] = ru;
        ++rank[ru];
      }
    }
  }
};

std::vector<ForwardingDAG>
getForwardingDAGConnectedComponents(ForwardingDAG &dag,
                                    std::vector<NumPyOptimizationUnit> &exprs) {
  auto n = exprs.size();
  UnionFind uf(n);

  for (auto i = 0; i < n; i++) {
    for (auto &fwd : dag[&exprs[i]]) {
      uf.union_(i, fwd.srcId);
    }
  }

  std::vector<std::vector<NumPyOptimizationUnit *>> components(n);
  for (auto i = 0; i < n; i++) {
    auto root = uf.find(i);
    components[root].push_back(&exprs[i]);
  }

  std::vector<ForwardingDAG> result;
  for (auto &c : components) {
    if (c.empty())
      continue;

    ForwardingDAG d;
    for (auto *expr : c)
      d.emplace(expr, dag[expr]);
    result.push_back(d);
  }

  return result;
}

bool hasCycleHelper(int64_t v, ForwardingDAG &dag,
                    std::vector<NumPyOptimizationUnit> &exprs,
                    std::vector<bool> &visited, std::vector<bool> &recStack) {
  visited[v] = true;
  recStack[v] = true;

  for (auto &neighbor : dag[&exprs[v]]) {
    if (!visited[neighbor.srcId]) {
      if (hasCycleHelper(neighbor.srcId, dag, exprs, visited, recStack))
        return true;
    } else if (recStack[neighbor.srcId]) {
      return true;
    }
  }

  recStack[v] = false;
  return false;
}

bool hasCycle(ForwardingDAG &dag, std::vector<NumPyOptimizationUnit> &exprs) {
  auto n = exprs.size();
  std::vector<bool> visited(n, false);
  std::vector<bool> recStack(n, false);

  for (auto i = 0; i < n; i++) {
    if (dag.find(&exprs[i]) != dag.end() && !visited[i] &&
        hasCycleHelper(i, dag, exprs, visited, recStack))
      return true;
  }
  return false;
}

void doForwardingHelper(ForwardingDAG &dag, NumPyOptimizationUnit *curr,
                        std::unordered_set<NumPyOptimizationUnit *> &done,
                        std::vector<AssignInstr *> &assignsToDelete) {
  if (done.count(curr))
    return;

  auto forwardings = dag[curr];
  for (auto &fwd : forwardings) {
    doForwardingHelper(dag, fwd.src, done, assignsToDelete);
    // Note that order of leaves here doesn't matter since they're guaranteed to have no
    // side effects based on forwarding checks.
    fwd.dst->leaves.insert(fwd.dst->leaves.end(), fwd.src->leaves.begin(),
                           fwd.src->leaves.end());
    fwd.dstLeaf->replace(*fwd.src->expr);
    assignsToDelete.push_back(fwd.src->assign);
  }

  done.insert(curr);
}
} // namespace

std::vector<ForwardingDAG>
getForwardingDAGs(BodiedFunc *func, RD *rd, CFG *cfg, SE *se,
                  std::vector<NumPyOptimizationUnit> &exprs) {
  auto dag = buildForwardingDAG(func, rd, cfg, se, exprs);
  auto dags = getForwardingDAGConnectedComponents(dag, exprs);
  dags.erase(std::remove_if(dags.begin(), dags.end(),
                            [&](ForwardingDAG &dag) { return hasCycle(dag, exprs); }),
             dags.end());
  return dags;
}

NumPyOptimizationUnit *doForwarding(ForwardingDAG &dag,
                                    std::vector<AssignInstr *> &assignsToDelete) {
  seqassertn(!dag.empty(), "empty forwarding DAG encountered");
  std::unordered_set<NumPyOptimizationUnit *> done;
  for (auto &e : dag) {
    doForwardingHelper(dag, e.first, done, assignsToDelete);
  }

  // Find the root
  std::unordered_set<NumPyOptimizationUnit *> notRoot;
  for (auto &e : dag) {
    for (auto &f : e.second) {
      notRoot.insert(f.src);
    }
  }
  seqassertn(notRoot.size() == dag.size() - 1,
             "multiple roots found in forwarding DAG");

  for (auto &e : dag) {
    if (notRoot.count(e.first) == 0)
      return e.first;
  }

  seqassertn(false, "could not find root in forwarding DAG");
  return nullptr;
}

} // namespace numpy
} // namespace transform
} // namespace ir
} // namespace codon
