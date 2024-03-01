// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "list.h"

#include <algorithm>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"

namespace codon {
namespace ir {
namespace transform {
namespace pythonic {
namespace {

static const std::string LIST = "std.internal.types.ptr.List";
static const std::string SLICE = "std.internal.types.slice.Slice[int,int,int]";

bool isList(Value *v) { return v->getType()->getName().rfind(LIST + "[", 0) == 0; }
bool isSlice(Value *v) { return v->getType()->getName() == SLICE; }

// The following "handlers" account for the possible sub-expressions we might
// see when optimizing list1 + list2 + ... listN. Currently, we optimize:
//   - Slices: x[a:b:c] (avoid constructing the temporary sliced list)
//   - Literals: [a, b, c] (just append elements directly)
//   - Default: <any list expr> (append by iterating over the list)
// It is easy to handle new sub-expression types by adding new handlers.
// There are three stages in the optimized code:
//   - Setup: assign all the relevant expressions to variables, making
//            sure they're evaluated in the same order as before
//   - Count: figure out the total length of the resulting list
//   - Create: initialize a new list with the appropriate capacity and
//             append all the elements
// The handlers have virtual functions to generate IR for each of these steps.

struct ElementHandler {
  std::vector<Var *> vars;

  ElementHandler() : vars() {}
  virtual ~ElementHandler() {}
  virtual void setup(SeriesFlow *block, BodiedFunc *parent) = 0;
  virtual Value *length(Module *M) = 0;
  virtual Value *append(Value *result) = 0;

  void doSetup(const std::vector<Value *> &values, SeriesFlow *block,
               BodiedFunc *parent) {
    for (auto *v : values) {
      vars.push_back(util::makeVar(v, block, parent)->getVar());
    }
  }

  static std::unique_ptr<ElementHandler> get(Value *v, types::Type *ty);
};

struct DefaultHandler : public ElementHandler {
  Value *element;

  DefaultHandler(Value *element) : ElementHandler(), element(element) {}

  void setup(SeriesFlow *block, BodiedFunc *parent) override {
    doSetup({element}, block, parent);
  }

  Value *length(Module *M) override {
    auto *e = M->Nr<VarValue>(vars[0]);
    auto *ty = element->getType();
    auto *fn = M->getOrRealizeMethod(ty, "_list_add_opt_default_len", {ty});
    seqassertn(fn, "could not find default list length helper");
    return util::call(fn, {e});
  }

  Value *append(Value *result) override {
    auto *M = result->getModule();
    auto *e = M->Nr<VarValue>(vars[0]);
    auto *ty = result->getType();
    auto *fn = M->getOrRealizeMethod(ty, "_list_add_opt_default_append", {ty, ty});
    seqassertn(fn, "could not find default list append helper");
    return util::call(fn, {result, e});
  }

  static std::unique_ptr<ElementHandler> get(Value *v, types::Type *ty) {
    if (!v->getType()->is(ty))
      return {};
    return std::make_unique<DefaultHandler>(v);
  }
};

struct SliceHandler : public ElementHandler {
  Value *element;
  Value *slice;

  SliceHandler(Value *element, Value *slice)
      : ElementHandler(), element(element), slice(slice) {}

  void setup(SeriesFlow *block, BodiedFunc *parent) override {
    doSetup({element, slice}, block, parent);
  }

  Value *length(Module *M) override {
    auto *e = M->Nr<VarValue>(vars[0]);
    auto *s = M->Nr<VarValue>(vars[1]);
    auto *ty = element->getType();
    auto *fn =
        M->getOrRealizeMethod(ty, "_list_add_opt_slice_len", {ty, slice->getType()});
    seqassertn(fn, "could not find slice list length helper");
    return util::call(fn, {e, s});
  }

  Value *append(Value *result) override {
    auto *M = result->getModule();
    auto *e = M->Nr<VarValue>(vars[0]);
    auto *s = M->Nr<VarValue>(vars[1]);
    auto *ty = result->getType();
    auto *fn = M->getOrRealizeMethod(ty, "_list_add_opt_slice_append",
                                     {ty, ty, slice->getType()});
    seqassertn(fn, "could not find slice list append helper");
    return util::call(fn, {result, e, s});
  }

  static std::unique_ptr<ElementHandler> get(Value *v, types::Type *ty) {
    if (!v->getType()->is(ty))
      return {};

    if (auto *c = cast<CallInstr>(v)) {
      auto *func = util::getFunc(c->getCallee());
      if (func && func->getUnmangledName() == Module::GETITEM_MAGIC_NAME &&
          std::distance(c->begin(), c->end()) == 2 && isList(c->front()) &&
          isSlice(c->back())) {
        return std::make_unique<SliceHandler>(c->front(), c->back());
      }
    }

    return {};
  }
};

struct LiteralHandler : public ElementHandler {
  std::vector<Value *> elements;

  LiteralHandler(std::vector<Value *> elements)
      : ElementHandler(), elements(std::move(elements)) {}

  void setup(SeriesFlow *block, BodiedFunc *parent) override {
    doSetup(elements, block, parent);
  }

  Value *length(Module *M) override { return M->getInt(elements.size()); }

  Value *append(Value *result) override {
    auto *M = result->getModule();
    auto *ty = result->getType();
    auto *block = M->Nr<SeriesFlow>();
    if (vars.empty())
      return block;
    auto *fn = M->getOrRealizeMethod(ty, "_list_add_opt_literal_append",
                                     {ty, elements[0]->getType()});
    seqassertn(fn, "could not find literal list append helper");
    for (auto *var : vars) {
      block->push_back(util::call(fn, {result, M->Nr<VarValue>(var)}));
    }
    return block;
  }

  static std::unique_ptr<ElementHandler> get(Value *v, types::Type *ty) {
    if (!v->getType()->is(ty))
      return {};

    if (auto *attr = v->getAttribute<ListLiteralAttribute>()) {
      std::vector<Value *> elements;
      for (auto &element : attr->elements) {
        if (element.star)
          return {};
        elements.push_back(element.value);
      }
      return std::make_unique<LiteralHandler>(std::move(elements));
    }

    return {};
  }
};

std::unique_ptr<ElementHandler> ElementHandler::get(Value *v, types::Type *ty) {
  if (auto h = SliceHandler::get(v, ty))
    return std::move(h);

  if (auto h = LiteralHandler::get(v, ty))
    return std::move(h);

  return DefaultHandler::get(v, ty);
}

struct InspectionResult {
  bool valid = true;
  std::vector<Value *> args;
};

void inspect(Value *v, InspectionResult &r) {
  // check if add first then go from there
  if (isList(v)) {
    if (auto *c = cast<CallInstr>(v)) {
      auto *func = util::getFunc(c->getCallee());
      if (func && func->getUnmangledName() == Module::ADD_MAGIC_NAME &&
          c->numArgs() == 2 && isList(c->front()) && isList(c->back())) {
        inspect(c->front(), r);
        inspect(c->back(), r);
        return;
      }
    }
    r.args.push_back(v);
  } else {
    r.valid = false;
  }
}

Value *optimize(BodiedFunc *parent, InspectionResult &r) {
  if (!r.valid || r.args.size() <= 1)
    return nullptr;

  auto *M = parent->getModule();
  auto *ty = r.args[0]->getType();
  util::CloneVisitor cv(M);
  std::vector<std::unique_ptr<ElementHandler>> handlers;

  for (auto *v : r.args) {
    handlers.push_back(ElementHandler::get(cv.clone(v), ty));
  }

  auto *opt = M->Nr<SeriesFlow>();
  auto *len = util::makeVar(M->getInt(0), opt, parent)->getVar();

  for (auto &h : handlers) {
    h->setup(opt, parent);
  }

  for (auto &h : handlers) {
    opt->push_back(M->Nr<AssignInstr>(len, *M->Nr<VarValue>(len) + *h->length(M)));
  }

  auto *fn = M->getOrRealizeMethod(ty, "_list_add_opt_opt_new", {M->getIntType()});
  seqassertn(fn, "could not find list new helper");
  auto *result =
      util::makeVar(util::call(fn, {M->Nr<VarValue>(len)}), opt, parent)->getVar();

  for (auto &h : handlers) {
    opt->push_back(h->append(M->Nr<VarValue>(result)));
  }

  return M->Nr<FlowInstr>(opt, M->Nr<VarValue>(result));
}
} // namespace

const std::string ListAdditionOptimization::KEY = "core-pythonic-list-addition-opt";

void ListAdditionOptimization::handle(CallInstr *v) {
  auto *M = v->getModule();

  auto *f = util::getFunc(v->getCallee());
  if (!f || f->getUnmangledName() != Module::ADD_MAGIC_NAME)
    return;

  InspectionResult r;
  inspect(v, r);
  auto *parent = cast<BodiedFunc>(getParentFunc());
  if (auto *opt = optimize(parent, r))
    v->replaceAll(opt);
}

} // namespace pythonic
} // namespace transform
} // namespace ir
} // namespace codon
