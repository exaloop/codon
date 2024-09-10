// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "attr.h"

namespace codon::ast {

const std::string Attr::Module = "module";
const std::string Attr::ParentClass = "parentClass";
const std::string Attr::Bindings = "bindings";

const std::string Attr::LLVM = "llvm";
const std::string Attr::Python = "python";
const std::string Attr::Atomic = "atomic";
const std::string Attr::Property = "property";
const std::string Attr::StaticMethod = "staticmethod";
const std::string Attr::Attribute = "__attribute__";
const std::string Attr::C = "C";

const std::string Attr::Internal = "__internal__";
const std::string Attr::HiddenFromUser = "__hidden__";
const std::string Attr::ForceRealize = "__force__";
const std::string Attr::RealizeWithoutSelf =
    "std.internal.attributes.realize_without_self.0:0";

const std::string Attr::CVarArg = ".__vararg__";
const std::string Attr::Method = ".__method__";
const std::string Attr::Capture = ".__capture__";
const std::string Attr::HasSelf = ".__hasself__";
const std::string Attr::IsGenerator = ".__generator__";

const std::string Attr::Extend = "extend";
const std::string Attr::Tuple = "tuple";
const std::string Attr::ClassDeduce = "deduce";
const std::string Attr::ClassNoTuple = "__notuple__";

const std::string Attr::Test = "std.internal.attributes.test.0:0";
const std::string Attr::Overload = "overload:0";
const std::string Attr::Export = "std.internal.attributes.export.0:0";

const std::string Attr::ClassMagic = "classMagic";
const std::string Attr::ExprSequenceItem = "exprSequenceItem";
const std::string Attr::ExprStarSequenceItem = "exprStarSequenceItem";
const std::string Attr::ExprList = "exprList";
const std::string Attr::ExprSet = "exprSet";
const std::string Attr::ExprDict = "exprDict";
const std::string Attr::ExprPartial = "exprPartial";
const std::string Attr::ExprDominated = "exprDominated";
const std::string Attr::ExprStarArgument = "exprStarArgument";
const std::string Attr::ExprKwStarArgument = "exprKwStarArgument";
const std::string Attr::ExprOrderedCall = "exprOrderedCall";
const std::string Attr::ExprExternVar = "exprExternVar";
const std::string Attr::ExprDominatedUndefCheck = "exprDominatedUndefCheck";
const std::string Attr::ExprDominatedUsed = "exprDominatedUsed";

} // namespace codon::ast
