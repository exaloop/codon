// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include "pyextension.h"

#include <algorithm>
#include <iterator>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/matching.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {
namespace {
const std::string PYTHON_MODULE = "std.internal.python";
const std::string EXPORT_ATTR = "std.internal.attributes.export";

void extensionWarning(const std::string &parent, const std::string &method,
                      types::Type *type, const SrcInfo &src) {
  compilationWarning("Python extension lowering: type '" + type->getName() +
                         "' does not have '" + method +
                         "' method; ignoring exported function '" + parent + "'",
                     src.file, src.line, src.col);
}

Func *generateExtensionFunc(Func *f) {
  auto *M = f->getModule();
  auto *cobj = M->getPointerType(M->getByteType());
  auto *ext = M->Nr<BodiedFunc>(".pyext_wrapper." + f->getName());
  auto numArgs = std::distance(f->arg_begin(), f->arg_end());

  if (numArgs <= 1) {
    ext->realize(M->getFuncType(cobj, {cobj, cobj}), {"self", "args"});
  } else {
    ext->realize(M->getFuncType(cobj, {cobj, M->getPointerType(cobj), M->getIntType()}),
                 {"self", "args", "nargs"});
  }

  auto *body = M->Nr<SeriesFlow>();
  ext->setBody(body);
  std::vector<Var *> extArgs(ext->arg_begin(), ext->arg_end());
  std::vector<Value *> vars;
  auto *args = extArgs[1];

  if (numArgs > 1) {
    auto *badArgsFn = M->getOrRealizeFunc(
        "_extension_bad_args", {M->getIntType(), M->getIntType()}, {}, PYTHON_MODULE);
    seqassertn(badArgsFn, "arg check function not found");
    auto *check =
        util::call(badArgsFn, {M->Nr<VarValue>(extArgs[2]), M->getInt(numArgs)});
    seqassertn(check->getType()->is(M->getBoolType()), "unexpected return type");
    auto *ifCheck = M->Nr<IfFlow>(check, util::series(M->Nr<ReturnInstr>((*cobj)())));
    body->push_back(ifCheck);
  }

  int idx = 0;
  for (auto it = f->arg_begin(); it != f->arg_end(); ++it) {
    auto *type = (*it)->getType();
    auto *fromPy = M->getOrRealizeMethod(type, "__from_py__", {cobj});
    if (!fromPy) {
      extensionWarning(f->getUnmangledName(), "__from_py__", type, f->getSrcInfo());
      M->remove(ext);
      return nullptr;
    }
    seqassertn(numArgs != 0, "unexpected argument");
    auto *argItem = (numArgs == 1) ? M->Nr<VarValue>(args)
                                   : (*M->Nr<VarValue>(args))[*M->getInt(idx++)];
    auto *pyItem = util::call(fromPy, {argItem});
    vars.push_back(util::makeVar(pyItem, body, ext));
  }

  auto *retType = util::getReturnType(f);
  auto *toPy = M->getOrRealizeMethod(retType, "__to_py__", {retType});
  if (!toPy) {
    extensionWarning(f->getUnmangledName(), "__to_py__", retType, f->getSrcInfo());
    M->remove(ext);
    return nullptr;
  }
  auto *retVal = util::call(toPy, {util::call(f, vars)});
  body->push_back(M->Nr<ReturnInstr>(retVal));
  return ext;
}
} // namespace

const std::string PythonExtensionLowering::KEY = "core-python-extension-lowering";

void PythonExtensionLowering::run(Module *module) {
  for (auto *var : *module) {
    if (auto *f = cast<BodiedFunc>(var)) {
      if (!util::hasAttribute(f, EXPORT_ATTR))
        continue;

      if (auto *g = generateExtensionFunc(f)) {
        LOG("[pyext] exporting {}", f->getName());
        g->setAttribute(std::make_unique<PythonWrapperAttribute>(f));
      }
    }
  }
}

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
