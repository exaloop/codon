// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

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
    return false;

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
        return true;
      }

      if (!go)
        continue;

      OkToForwardPast check(vids, parsedValues, se);
      const_cast<Value *>(value)->accept(check);
      if (!check.ok)
        return false;
    }
  }
  return false;
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
  // A consumer must not run again without reexecuting its producer. Single-use
  // syntax alone does not prove this when the consumer is inside a loop.
  std::unordered_set<CFBlock *> visited;
  std::vector<CFBlock *> pending(end->successors_begin(), end->successors_end());
  while (!pending.empty()) {
    auto *curr = pending.back();
    pending.pop_back();
    if (curr == start)
      continue;
    if (curr == end)
      return false;
    if (visited.insert(curr).second)
      pending.insert(pending.end(), curr->successors_begin(), curr->successors_end());
  }

  bool ok = true;
  bool reached = false;

  // Every path must reach the consumer, or forwarding could suppress an eager
  // shape error. Also reject cycles and intervening writes/effects.
  std::function<void(CFBlock *, std::vector<CFBlock *> &)> dfs =
      [&](CFBlock *curr, std::vector<CFBlock *> &path) {
        if (!ok)
          return;
        path.push_back(curr);
        if (curr == end) {
          reached = true;
          if (!canForwardExpressionAlongPath(source, target, vids, parsedValues, se,
                                             path))
            ok = false;
        } else {
          if (curr->successors_begin() == curr->successors_end())
            ok = false;
          for (auto it = curr->successors_begin(); it != curr->successors_end(); ++it) {
            if (std::find(path.begin(), path.end(), *it) != path.end()) {
              ok = false;
              break;
            }
            dfs(*it, path);
          }
        }
        path.pop_back();
      };

  std::vector<CFBlock *> path;
  dfs(start, path);
  return ok && reached;
}

bool canForwardVariable(AssignInstr *assign, Value *destination, BodiedFunc *func,
                        RD *rd) {
  auto *var = assign->getLhs();

  // Check 1: Only the given assignment should reach the destination.
  auto reaching = rd->getReachingDefinitions(var, destination);
  if (reaching.size() != 1 || reaching[0].assignment->getId() != assign->getId())
    return false;

  // Check 2: There should be no other uses reached by this assignment. These are
  // individual references, so even repeated operands in one consumer disqualify it.
  std::vector<Value *> uses;
  GetAllUses gu(var, uses);
  func->accept(gu);
  for (auto *use : uses) {
    if (use != destination && use->getId() != assign->getId()) {
      auto defs = rd->getReachingDefinitions(var, use);
      for (auto &def : defs) {
        if (def.assignment->getId() == assign->getId())
          return false;
      }
    }
  }

  return true;
}

// Edges point from a consumer to a producer that can replace one of its leaves.
// Reaching definitions establish single-use; CFG/effect checks require the consumer
// on every path and reject intervening writes before moving the producer.
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
          // Keep reductions materialized. Their buffers can be reused later
          // without moving the reduction itself into another expression.
          if (checkFwdVar && !src.expr->isReduction() &&
              canForwardExpression(&src, p.second->val, parsedValues, cfg, se)) {
            forwardingVec.push_back({&dst, &src, p.first, p.second, dstId, srcId});
          }
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
                        std::vector<AssignInstr *> &assignsToDelete,
                        std::vector<std::pair<Value *, Value *>> *substitutions) {
  if (done.count(curr))
    return;

  auto forwardings = dag[curr];
  for (auto &fwd : forwardings) {
    doForwardingHelper(dag, fwd.src, done, assignsToDelete, substitutions);
    // Note that order of leaves here doesn't matter since they're guaranteed to have no
    // side effects based on forwarding checks.
    fwd.dst->leaves.insert(fwd.dst->leaves.end(), fwd.src->leaves.begin(),
                           fwd.src->leaves.end());
    if (substitutions)
      substitutions->emplace_back(fwd.dstLeaf->val, fwd.src->value);
    fwd.dstLeaf->replace(*fwd.src->expr);
    assignsToDelete.push_back(fwd.src->assign);
  }

  done.insert(curr);
}

NumPyOptimizationUnit *getForwardingRoot(ForwardingDAG &dag) {
  std::unordered_set<NumPyOptimizationUnit *> notRoot;
  for (auto &entry : dag) {
    for (auto &forwarding : entry.second)
      notRoot.insert(forwarding.src);
  }
  seqassertn(notRoot.size() == dag.size() - 1,
             "multiple roots found in forwarding DAG");
  for (auto &entry : dag) {
    if (!notRoot.count(entry.first))
      return entry.first;
  }
  seqassertn(false, "could not find root in forwarding DAG");
  return nullptr;
}

// Allow a final pointwise consumer to reuse a buffer after earlier reductions
// have finished reading it. Any other reached use prevents this ownership transfer.
bool canReuseAfterReads(NumPyOptimizationUnit &source, NumPyExpr &destination,
                        NumPyOptimizationUnit &consumer,
                        std::vector<NumPyOptimizationUnit> &exprs, CFG *cfg, RD *rd,
                        SE *se) {
  if (consumer.expr->isReduction() || consumer.expr->depth() != 2 ||
      (&destination != consumer.expr->lhs.get() &&
       &destination != consumer.expr->rhs.get()))
    return false;
  auto *assign = source.assign;
  auto *var = assign->getLhs();
  auto reaching = rd->getReachingDefinitions(var, destination.val);
  if (reaching.size() != 1 || reaching[0].assignment->getId() != assign->getId())
    return false;

  auto *block = cfg->getBlock(assign);
  std::unordered_map<id_t, size_t> positions;
  for (const auto *value : *block)
    positions.emplace(value->getId(), positions.size());
  auto target = positions.find(destination.val->getId());
  if (target == positions.end())
    return false;

  std::unordered_set<id_t> completedReads;
  for (auto &reader : exprs) {
    auto &expr = *reader.expr;
    if (!expr.isReduction() || !expr.lhs->isLeaf())
      continue;
    auto position = positions.find(reader.value->getId());
    if (position == positions.end() ||
        position->second <= positions.at(assign->getId()) ||
        position->second >= target->second)
      continue;
    auto *operand = cast<VarValue>(expr.lhs->val);
    auto *call = cast<CallInstr>(expr.val);
    if (!operand || operand->getVar() != var || !call)
      continue;
    bool pureArguments = true;
    for (auto argument = std::next(call->begin()); argument != call->end(); ++argument)
      pureArguments &= !se->hasSideEffect(*argument);
    if (pureArguments)
      completedReads.insert(operand->getId());
  }
  if (completedReads.empty())
    return false;

  // Alias/view creation is also a use. Reject it even if the alias is not returned:
  // it could mutate or retain the allocation without reading this variable again.
  std::vector<Value *> uses;
  GetAllUses collect(var, uses);
  consumer.func->accept(collect);
  for (auto *use : uses) {
    if (use == destination.val || use->getId() == assign->getId())
      continue;
    for (auto &definition : rd->getReachingDefinitions(var, use)) {
      if (definition.assignment->getId() == assign->getId() &&
          !completedReads.count(use->getId()))
        return false;
    }
  }
  return true;
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
  // Transfer ownership separately from expression forwarding. An ownedLastUse leaf
  // proves ownership and no subsequent alias access, permitting reuse or release
  // without moving the source computation or adding a forwarding edge.
  for (auto &component : dags) {
    auto *root = getForwardingRoot(component);
    auto *block = cfg->getBlock(root->value);
    for (auto &source : exprs) {
      if (!block || !source.assign || component.count(&source) ||
          !hasOwnedResult(*source.expr) || source.assign->getLhs()->isGlobal() ||
          cfg->getBlock(source.assign) != block)
        continue;
      bool available = false;
      for (const auto *value : *block) {
        if (value == root->value)
          break;
        if (value == source.assign)
          available = true;
      }
      if (!available)
        continue;
      for (auto &entry : component) {
        entry.first->expr->apply([&](NumPyExpr &element) {
          auto *variable = element.isLeaf() ? cast<VarValue>(element.val) : nullptr;
          if (variable && variable->getVar() == source.assign->getLhs() &&
              ((source.expr->isReduction() &&
                canForwardVariable(source.assign, element.val, func, rd)) ||
               (entry.first == root &&
                canReuseAfterReads(source, element, *root, exprs, cfg, rd, se))))
            element.ownedLastUse = true;
        });
      }
    }
  }
  return dags;
}

NumPyOptimizationUnit *
doForwarding(ForwardingDAG &dag, std::vector<AssignInstr *> &assignsToDelete,
             std::vector<std::pair<Value *, Value *>> *substitutions) {
  seqassertn(!dag.empty(), "empty forwarding DAG encountered");
  std::unordered_set<NumPyOptimizationUnit *> done;
  for (auto &e : dag) {
    doForwardingHelper(dag, e.first, done, assignsToDelete, substitutions);
  }

  return getForwardingRoot(dag);
}

} // namespace numpy
} // namespace transform
} // namespace ir
} // namespace codon
