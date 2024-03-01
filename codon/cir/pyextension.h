// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <vector>

#include "codon/cir/func.h"
#include "codon/cir/types/types.h"

namespace codon {
namespace ir {

struct PyFunction {
  enum Type { TOPLEVEL, METHOD, CLASS, STATIC };
  std::string name;
  std::string doc;
  Func *func = nullptr;
  Type type = Type::TOPLEVEL;
  int nargs = 0;
  bool keywords = false;
  bool coexist = false;
};

struct PyMember {
  enum Type {
    SHORT = 0,
    INT = 1,
    LONG = 2,
    FLOAT = 3,
    DOUBLE = 4,
    STRING = 5,
    OBJECT = 6,
    CHAR = 7,
    BYTE = 8,
    UBYTE = 9,
    USHORT = 10,
    UINT = 11,
    ULONG = 12,
    STRING_INPLACE = 13,
    BOOL = 14,
    OBJECT_EX = 16,
    LONGLONG = 17,
    ULONGLONG = 18,
    PYSSIZET = 19,
  };

  std::string name;
  std::string doc;
  Type type = Type::SHORT;
  bool readonly = false;
  /// Indexes of the member. For example, in the
  /// tuple (a, (b, c, (d,))), 'a' would have indexes
  /// [0], 'b' would have indexes [1, 0], 'c' would
  /// have indexes [1, 1], and 'd' would have indexes
  /// [1, 2, 0]. This corresponds to an LLVM GEP.
  std::vector<int> indexes;
};

struct PyGetSet {
  std::string name;
  std::string doc;
  Func *get = nullptr;
  Func *set = nullptr;
};

struct PyType {
  std::string name;
  std::string doc;
  types::Type *type = nullptr;
  PyType *base = nullptr;
  Func *repr = nullptr;
  Func *add = nullptr;
  Func *iadd = nullptr;
  Func *sub = nullptr;
  Func *isub = nullptr;
  Func *mul = nullptr;
  Func *imul = nullptr;
  Func *mod = nullptr;
  Func *imod = nullptr;
  Func *divmod = nullptr;
  Func *pow = nullptr;
  Func *ipow = nullptr;
  Func *neg = nullptr;
  Func *pos = nullptr;
  Func *abs = nullptr;
  Func *bool_ = nullptr;
  Func *invert = nullptr;
  Func *lshift = nullptr;
  Func *ilshift = nullptr;
  Func *rshift = nullptr;
  Func *irshift = nullptr;
  Func *and_ = nullptr;
  Func *iand = nullptr;
  Func *xor_ = nullptr;
  Func *ixor = nullptr;
  Func *or_ = nullptr;
  Func *ior = nullptr;
  Func *int_ = nullptr;
  Func *float_ = nullptr;
  Func *floordiv = nullptr;
  Func *ifloordiv = nullptr;
  Func *truediv = nullptr;
  Func *itruediv = nullptr;
  Func *index = nullptr;
  Func *matmul = nullptr;
  Func *imatmul = nullptr;
  Func *len = nullptr;
  Func *getitem = nullptr;
  Func *setitem = nullptr;
  Func *contains = nullptr;
  Func *hash = nullptr;
  Func *call = nullptr;
  Func *str = nullptr;
  Func *cmp = nullptr;
  Func *iter = nullptr;
  Func *iternext = nullptr;
  Func *del = nullptr;
  Func *init = nullptr;
  std::vector<PyFunction> methods;
  std::vector<PyMember> members;
  std::vector<PyGetSet> getset;
  Func *typePtrHook = nullptr;
};

struct PyModule {
  std::string name;
  std::string doc;
  std::vector<PyFunction> functions;
  std::vector<PyType> types;
};

} // namespace ir
} // namespace codon
