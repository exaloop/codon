// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include "pyextension.h"

#include <algorithm>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/matching.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {
namespace {

const std::string EXPORT_ATTR = "std.internal.attributes.export";

Func *generateExtensionFunc(Func *f) {
  //  PyObject *_PyCFunctionFast(PyObject *self,
  //                             PyObject *const *args,
  //                             Py_ssize_t nargs);

  auto *M = f->getModule();
  auto *cobj = M->getPointerType(M->getByteType());
  auto *ext = M->Nr<BodiedFunc>("__py_extension");
  ext->realize(M->getFuncType(cobj, {cobj, M->getPointerType(cobj), M->getIntType()}),
               {"self", "args", "nargs"});
  auto *body = M->Nr<SeriesFlow>();
  ext->setBody(body);
  std::vector<Var *> extArgs(ext->arg_begin(), ext->arg_end());
  std::vector<Value *> vars;
  auto *args = extArgs[1];
  // auto *nargs = extArgs[2];

  // TODO: check nargs

  int idx = 0;
  for (auto it = f->arg_begin(); it != f->arg_end(); ++it) {
    auto *type = (*it)->getType();
    auto *fromPy = M->getOrRealizeMethod(type, "__from_py__", {cobj});
    seqassertn(fromPy, "__from_py__ method not found");
    auto *pyItem = util::call(fromPy, {(*M->Nr<VarValue>(args))[*M->getInt(idx++)]});
    vars.push_back(util::makeVar(pyItem, body, ext));
  }

  auto *retType = util::getReturnType(f);
  auto *toPy = M->getOrRealizeMethod(retType, "__to_py__", {retType});
  seqassertn(toPy, "__to_py__ method not found");
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

      std::cout << f->getName() << std::endl;
      std::cout << *generateExtensionFunc(f) << std::endl;
    }
  }
}

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
