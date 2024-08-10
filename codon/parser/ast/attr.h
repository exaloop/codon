// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>

namespace codon::ast {

const int INDENT_SIZE = 2;

struct Attr {
  // Function attributes
  const static std::string Module;
  const static std::string ParentClass;
  const static std::string Bindings;
  // Toplevel attributes
  const static std::string LLVM;
  const static std::string Python;
  const static std::string Atomic;
  const static std::string Property;
  const static std::string StaticMethod;
  const static std::string Attribute;
  const static std::string C;
  // Internal attributes
  const static std::string Internal;
  const static std::string HiddenFromUser;
  const static std::string ForceRealize;
  const static std::string RealizeWithoutSelf; // not internal
  // Compiler-generated attributes
  const static std::string CVarArg;
  const static std::string Method;
  const static std::string Capture;
  const static std::string HasSelf;
  const static std::string IsGenerator;
  // Class attributes
  const static std::string Extend;
  const static std::string Tuple;
  const static std::string ClassDeduce;
  const static std::string ClassNoTuple;
  // Standard library attributes
  const static std::string Test;
  const static std::string Overload;
  const static std::string Export;
  // Expression-related attributes
  const static std::string ClassMagic;
  const static std::string ExprSequenceItem;
  const static std::string ExprStarSequenceItem;
  const static std::string ExprList;
  const static std::string ExprSet;
  const static std::string ExprDict;
  const static std::string ExprPartial;
  const static std::string ExprDominated;
  const static std::string ExprStarArgument;
  const static std::string ExprKwStarArgument;
  const static std::string ExprOrderedCall;
  const static std::string ExprExternVar;
  const static std::string ExprDominatedUndefCheck;
  const static std::string ExprDominatedUsed;
};
}