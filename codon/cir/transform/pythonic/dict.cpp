// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "dict.h"

#include <algorithm>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/matching.h"

namespace codon {
namespace ir {
namespace transform {
namespace pythonic {
namespace {
/// get or __getitem__ call metadata
struct GetCall {
  /// the function, nullptr if not a get call
  Func *func = nullptr;
  /// the dictionary, must not be a call
  Value *dict = nullptr;
  /// the key, must not be a call
  Value *key = nullptr;
  /// the default value, may be null
  Const *dflt = nullptr;
};

/// Identify the call and return its metadata.
/// @param call the call
/// @return the metadata
GetCall analyzeGet(CallInstr *call) {
  // extract the function
  auto *func = util::getFunc(call->getCallee());
  if (!func)
    return {};

  auto unmangled = func->getUnmangledName();

  // canonical get/__getitem__ calls have at least two arguments
  auto it = call->begin();
  auto dist = std::distance(it, call->end());
  if (dist < 2)
    return {};

  // extract the dictionary and keys
  auto *dict = *it++;
  auto *k = *it++;

  // dictionary and key must not be calls
  if (isA<CallInstr>(dict) || isA<CallInstr>(k))
    return {};

  // get calls have a default
  if (unmangled == "get" && std::distance(it, call->end()) == 1) {
    auto *dflt = cast<Const>(*it);
    return {func, dict, k, dflt};
  } else if (unmangled == "__getitem__" && std::distance(it, call->end()) == 0) {
    return {func, dict, k, nullptr};
  }

  // call is not correct
  return {};
}
} // namespace

const std::string DictArithmeticOptimization::KEY = "core-pythonic-dict-arithmetic-opt";

void DictArithmeticOptimization::handle(CallInstr *v) {
  auto *M = v->getModule();

  // get and check the exterior function (should be a __setitem__ with 3 args)
  auto *setFunc = util::getFunc(v->getCallee());
  if (setFunc && setFunc->getUnmangledName() == "__setitem__" &&
      std::distance(v->begin(), v->end()) == 3) {
    auto it = v->begin();

    // extract all the arguments to the function
    // the dictionary and key must not be calls, and the value must
    // be a call
    auto *dictValue = *it++;
    auto *keyValue = *it++;
    if (isA<CallInstr>(dictValue) || isA<CallInstr>(keyValue))
      return;
    auto *opCall = cast<CallInstr>(*it++);

    // the call must take exactly two arguments
    if (!dictValue || !opCall || std::distance(opCall->begin(), opCall->end()) != 2)
      return;

    // grab the function, which needs to be an int or float call for now
    auto *opFunc = util::getFunc(opCall->getCallee());
    auto *getCall = cast<CallInstr>(opCall->front());
    if (!opFunc || !getCall)
      return;

    auto *intType = M->getIntType();
    auto *floatType = M->getFloatType();
    auto *parentType = opFunc->getParentType();
    if (!parentType || !(parentType->is(intType) || parentType->is(floatType)))
      return;

    // check the first argument
    auto getAnalysis = analyzeGet(getCall);
    if (!getAnalysis.func)
      return;

    // second argument can be any non-null value
    auto *secondValue = opCall->back();

    // verify that we are dealing with the same dictionary and key
    if (util::match(dictValue, getAnalysis.dict, false, true) &&
        util::match(keyValue, getAnalysis.key, false, true)) {
      util::CloneVisitor cv(M);
      Func *replacementFunc;

      // call non-throwing version if we have a default
      if (getAnalysis.dflt) {
        replacementFunc = M->getOrRealizeMethod(
            dictValue->getType(), "__dict_do_op__",
            {dictValue->getType(), keyValue->getType(), secondValue->getType(),
             getAnalysis.dflt->getType(), opFunc->getType()});
      } else {
        replacementFunc =
            M->getOrRealizeMethod(dictValue->getType(), "__dict_do_op_throws__",
                                  {dictValue->getType(), keyValue->getType(),
                                   secondValue->getType(), opFunc->getType()});
      }

      if (replacementFunc) {
        std::vector<Value *> args = {cv.clone(dictValue), cv.clone(keyValue),
                                     cv.clone(secondValue)};
        if (getAnalysis.dflt)
          args.push_back(cv.clone(getAnalysis.dflt));

        // sanity check to make sure function is inlined
        if (args.size() !=
            std::distance(replacementFunc->arg_begin(), replacementFunc->arg_end()))
          args.push_back(M->N<VarValue>(v, opFunc));

        v->replaceAll(util::call(replacementFunc, args));
      }
    }
  }
}

} // namespace pythonic
} // namespace transform
} // namespace ir
} // namespace codon
