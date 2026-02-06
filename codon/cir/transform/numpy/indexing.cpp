// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "indexing.h"

#include "codon/cir/analyze/dataflow/reaching.h"
#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"

#include <algorithm>

namespace codon {
namespace ir {
namespace transform {
namespace numpy {
namespace {
const std::string FUSION_MODULE = "std.numpy.fusion";

struct Term {
  enum Kind { INT, VAR, LEN } kind;
  int64_t val;
  const VarValue *var;

  Term(Kind kind, int64_t val, const VarValue *var) : kind(kind), val(val), var(var) {}

  static Term valTerm(int64_t v) { return {Kind::INT, v, nullptr}; }

  static Term varTerm(VarValue *v) { return {Kind::VAR, 1, v}; }

  static Term lenTerm(VarValue *v) { return {Kind::LEN, 1, v}; }

  void negate() { val = -val; }

  void multiply(int64_t n) { val *= n; }

  bool combine(const Term &other) {
    if (kind == other.kind &&
        (kind == Kind::INT || var->getVar()->getId() == other.var->getVar()->getId())) {
      val += other.val;
      return true;
    }
    return false;
  }

  bool zero() const { return val == 0; }

  std::string str() const {
    switch (kind) {
    case Kind::INT:
      return std::to_string(val);
    case Kind::VAR:
      return (val == 1 ? "" : std::to_string(val) + "*") + var->getVar()->getName();
    case Kind::LEN:
      return (val == 1 ? "" : std::to_string(val) + "*") + "len(" +
             var->getVar()->getName() + ")";
    }
    return "(?)";
  }
};

std::string t2s(const std::vector<Term> &terms) {
  if (terms.empty()) {
    return "[]";
  }
  std::string s = "[" + terms[0].str();
  for (auto i = 1; i < terms.size(); i++)
    s += ", " + terms[i].str();
  s += "]";
  return s;
}

void simplify(std::vector<Term> &terms) {
  // Presumably the number of terms will be small,
  // so we use a simple quadratic algorithm.
  for (auto it1 = terms.begin(); it1 != terms.end(); ++it1) {
    auto it2 = it1 + 1;
    while (it2 != terms.end()) {
      auto &t1 = *it1;
      auto &t2 = *it2;
      if (t1.combine(t2)) {
        it2 = terms.erase(it2);
      } else {
        ++it2;
      }
    }
  }

  terms.erase(std::remove_if(terms.begin(), terms.end(),
                             [](const Term &t) { return t.zero(); }),
              terms.end());
}

bool checkTotal(const std::vector<Term> &terms, bool strict) {
  int64_t total = 0;
  for (const auto &term : terms) {
    switch (term.kind) {
    case Term::Kind::INT:
      total += term.val;
      break;
    case Term::Kind::VAR:
      if (term.val != 0)
        return false;
      break;
    case Term::Kind::LEN:
      // len is never negative
      break;
    }
  }
  return strict ? (total > 0) : (total >= 0);
}

bool lessCheck(const std::vector<Term> &terms1, const std::vector<Term> &terms2,
               bool strict) {
  // Checking if: t1_0 + t1_1 + ... + t1_N < t2_0 + t2_1 + ... + t2_M
  // Same as: t2_0 + t2_1 + ... + t2_M - t1_0 - t1_1 - ... - t1_N > 0
  std::vector<Term> tmp(terms1.begin(), terms1.end());
  for (auto &t : tmp)
    t.negate();

  std::vector<Term> terms(terms2.begin(), terms2.end());
  terms.insert(terms.end(), tmp.begin(), tmp.end());
  simplify(terms);
  return checkTotal(terms, strict);
}

bool lessThan(const std::vector<Term> &terms1, const std::vector<Term> &terms2) {
  return lessCheck(terms1, terms2, /*strict=*/true);
}

bool lessThanOrEqual(const std::vector<Term> &terms1, const std::vector<Term> &terms2) {
  return lessCheck(terms1, terms2, /*strict=*/false);
}

std::vector<Term> replaceLoopVariable(const std::vector<Term> &terms, Var *loopVar,
                                      const std::vector<Term> &replacement) {
  std::vector<Term> ans;
  for (auto &term : terms) {
    if (term.kind == Term::Kind::VAR &&
        term.var->getVar()->getId() == loopVar->getId()) {
      for (auto &rep : replacement) {
        ans.push_back(rep);
        ans.back().multiply(term.val);
      }
    } else {
      ans.push_back(term);
    }
  }
  return ans;
}

bool isArrayType(types::Type *t, bool dim1 = false) {
  bool result = t && isA<types::RecordType>(t) &&
                t->getName().rfind(
                    ast::getMangledClass("std.numpy.ndarray", "ndarray") + "[", 0) == 0;
  if (result && dim1) {
    auto generics = t->getGenerics();
    seqassertn(generics.size() == 2 && generics[0].isType() && generics[1].isStatic(),
               "unrecognized ndarray generics");
    auto ndim = generics[1].getStaticValue();
    result &= (ndim == 1);
  }
  return result;
}

bool isLen(Func *f) {
  return f->getName().rfind(ast::getMangledFunc("std.internal.builtin", "len") + "[",
                            0) == 0;
}

bool parse(Value *x, std::vector<Term> &terms, bool negate = false) {
  auto push = [&](const Term &t) {
    terms.push_back(t);
    if (negate)
      terms.back().negate();
  };

  if (auto *v = cast<IntConst>(x)) {
    push(Term::valTerm(v->getVal()));
    return true;
  }

  if (auto *v = cast<VarValue>(x)) {
    push(Term::varTerm(v));
    return true;
  }

  auto *M = x->getModule();
  auto *intType = M->getIntType();

  if (auto *v = cast<CallInstr>(x)) {
    if (util::isCallOf(v, Module::ADD_MAGIC_NAME, {intType, intType}, intType,
                       /*method=*/true)) {
      return parse(v->front(), terms, negate) && parse(v->back(), terms, negate);
    }

    if (util::isCallOf(v, Module::SUB_MAGIC_NAME, {intType, intType}, intType,
                       /*method=*/true)) {
      return parse(v->front(), terms, negate) && parse(v->back(), terms, !negate);
    }

    if (v->numArgs() == 1 && isArrayType(v->front()->getType()) &&
        isA<VarValue>(v->front()) &&
        util::isCallOf(v, "size", {v->front()->getType()}, intType, /*method=*/true)) {
      push(Term::lenTerm(cast<VarValue>(v->front())));
      return true;
    }

    if (v->numArgs() == 1 && isArrayType(v->front()->getType()) &&
        isA<VarValue>(v->front()) &&
        util::isCallOf(v, "len", {v->front()->getType()}, intType, /*method=*/false) &&
        isLen(util::getFunc(v->getCallee()))) {
      push(Term::lenTerm(cast<VarValue>(v->front())));
      return true;
    }
  }

  return false;
}

struct IndexInfo {
  CallInstr *orig;
  VarValue *arr;
  Value *idx;
  Value *item;

  IndexInfo(CallInstr *orig, VarValue *arr, Value *idx, Value *item)
      : orig(orig), arr(arr), idx(idx), item(item) {}
};

struct FindArrayIndex : public util::Operator {
  std::vector<IndexInfo> indexes;

  FindArrayIndex() : util::Operator(/*childrenFirst=*/true), indexes() {}

  void handle(CallInstr *v) override {
    if (v->numArgs() < 1 || !isArrayType(v->front()->getType(), /*dim1=*/true) ||
        !isA<VarValue>(v->front()))
      return;

    auto *M = v->getModule();
    auto *arrType = v->front()->getType();
    auto *arrVar = cast<VarValue>(v->front());
    auto *intType = v->getModule()->getIntType();

    if (util::isCallOf(v, Module::GETITEM_MAGIC_NAME, {arrType, intType},
                       /*output=*/nullptr,
                       /*method=*/true)) {
      indexes.emplace_back(v, arrVar, v->back(), nullptr);
    } else if (util::isCallOf(v, Module::SETITEM_MAGIC_NAME,
                              {arrType, intType, v->back()->getType()},
                              /*output=*/nullptr,
                              /*method=*/true)) {
      indexes.emplace_back(v, arrVar, *(v->begin() + 1), v->back());
    }
  }
};

void elideBoundsCheck(IndexInfo &index) {
  auto *M = index.orig->getModule();
  util::CloneVisitor cv(M);

  if (index.item) {
    auto *setitem = M->getOrRealizeFunc(
        "_array1d_set_nocheck",
        {index.arr->getType(), M->getIntType(), index.item->getType()}, {},
        FUSION_MODULE);
    seqassertn(setitem, "setitem function not found");
    index.orig->replaceAll(
        util::call(setitem, {M->Nr<VarValue>(index.arr->getVar()), cv.clone(index.idx),
                             cv.clone(index.item)}));
  } else {
    auto *getitem =
        M->getOrRealizeFunc("_array1d_get_nocheck",
                            {index.arr->getType(), M->getIntType()}, {}, FUSION_MODULE);
    seqassertn(getitem, "getitem function not found");
    index.orig->replaceAll(util::call(
        getitem, {M->Nr<VarValue>(index.arr->getVar()), cv.clone(index.idx)}));
  }
}

bool isOriginalLoopVar(const Value *loc, ImperativeForFlow *loop,
                       analyze::dataflow::RDInspector *rd) {
  // The loop variable should have exactly two reaching definitions:
  //   - The initial assignment for the loop
  //   - The update assignment
  // Both are represented as `SyntheticAssignInstr` in the CFG.
  auto *loopVar = loop->getVar();
  auto defs = rd->getReachingDefinitions(loopVar, loc);
  if (defs.size() != 2)
    return false;

  using SAI = analyze::dataflow::SyntheticAssignInstr;
  auto *s1 = cast<SAI>(defs[0].assignment);
  auto *s2 = cast<SAI>(defs[1].assignment);

  if (!s1 || !s2)
    return false;

  if (s1->getKind() == SAI::Kind::ADD && s2->getKind() == SAI::Kind::KNOWN) {
    auto *tmp = s1;
    s1 = s2;
    s2 = tmp;
  } else if (!(s1->getKind() == SAI::Kind::KNOWN && s2->getKind() == SAI::Kind::ADD)) {
    return false;
  }

  auto *loop1 = cast<ImperativeForFlow>(s1->getSource());
  auto *loop2 = cast<ImperativeForFlow>(s2->getSource());

  if (!loop1 || !loop2 || loop1->getId() != loop->getId() ||
      loop2->getId() != loop->getId())
    return false;

  return true;
}

const VarValue *isAliasOfLoopVar(const VarValue *v, ImperativeForFlow *loop,
                                 analyze::dataflow::RDInspector *rd) {
  auto defs = rd->getReachingDefinitions(v->getVar(), v);
  auto *loopVar = loop->getVar();

  if (defs.size() != 1 || !defs[0].known() || !isA<VarValue>(defs[0].assignee) ||
      cast<VarValue>(defs[0].assignee)->getVar()->getId() != loopVar->getId() ||
      !isOriginalLoopVar(v, loop, rd))
    return nullptr;

  return cast<VarValue>(defs[0].assignee);
}

bool canElideBoundsCheck(ImperativeForFlow *loop, IndexInfo &index,
                         const std::vector<Term> &startTerms,
                         const std::vector<Term> &stopTerms,
                         analyze::dataflow::RDInspector *rd) {
  auto *loopVar = loop->getVar();
  std::vector<Term> idxTerms;
  if (!parse(index.idx, idxTerms))
    return false;

  // First, check that all involved variables refer to a consistent
  // value. We do this by making sure there is just one reaching def
  // for all VarValues referring to the same Var.
  std::unordered_map<id_t, id_t> reach; // "[var id] -> [reaching def id]" map
  auto check = [&](const VarValue *v) {
    auto id = v->getVar()->getId();
    if (id == loopVar->getId()) {
      return isOriginalLoopVar(v, loop, rd);
    } else {
      auto defs = rd->getReachingDefinitions(v->getVar(), v);
      if (defs.size() != 1)
        return false;

      auto rid = defs[0].getId();
      auto it = reach.find(id);

      if (it == reach.end()) {
        reach.emplace(id, rid);
        return true;
      } else {
        return it->second == rid;
      }
    }
  };

  if (!check(index.arr))
    return false;

  for (auto &term : startTerms) {
    if (term.kind != Term::Kind::INT && !check(term.var))
      return false;
  }

  for (auto &term : stopTerms) {
    if (term.kind != Term::Kind::INT && !check(term.var))
      return false;
  }

  for (auto &term : idxTerms) {
    if (term.kind != Term::Kind::INT && !check(term.var))
      return false;
  }

  // Update vars that are aliases of the loop var. This can
  // happen in e.g. compound assignments which need to create
  // a temporary copy of the index.
  for (auto &term : idxTerms) {
    if (term.kind != Term::Kind::VAR)
      continue;
    if (auto *loopVarValue = isAliasOfLoopVar(term.var, loop, rd))
      term.var = loopVarValue;
  }

  // Next, see if we can prove that indexes are in range.
  std::vector<Term> limit = {Term::lenTerm(index.arr)};
  auto terms1 = replaceLoopVariable(idxTerms, loopVar, startTerms);
  auto terms2 = replaceLoopVariable(idxTerms, loopVar, stopTerms);

  if (loop->getStep() > 0) {
    return lessThanOrEqual({Term::valTerm(0)}, terms1) &&
           lessThanOrEqual(terms2, limit);
  } else {
    return lessThan(terms1, limit) && lessThanOrEqual({Term::valTerm(-1)}, terms2);
  }
}

} // namespace

void NumPyBoundsCheckElisionPass::visit(ImperativeForFlow *f) {
  if (f->getStep() == 0 || f->getVar()->isGlobal())
    return;

  std::vector<Term> startTerms;
  std::vector<Term> stopTerms;
  FindArrayIndex find;
  f->getBody()->accept(find);

  if (find.indexes.empty() || !parse(f->getStart(), startTerms) ||
      !parse(f->getEnd(), stopTerms))
    return;

  auto *r = getAnalysisResult<analyze::dataflow::RDResult>(reachingDefKey);
  auto *c = r->cfgResult;
  auto it = r->results.find(getParentFunc()->getId());
  if (it == r->results.end())
    return;
  auto *rd = it->second.get();

  for (auto &index : find.indexes) {
    if (canElideBoundsCheck(f, index, startTerms, stopTerms, rd)) {
      elideBoundsCheck(index);
    }
  }
}

const std::string NumPyBoundsCheckElisionPass::KEY = "core-numpy-bounds-check-elision";

} // namespace numpy
} // namespace transform
} // namespace ir
} // namespace codon
