// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "codon/parser/ast/error.h"
#include "llvm/Support/Error.h"
#include <fmt/format.h>

namespace codon {
namespace error {

enum Error {
  CALL_NAME_ORDER,
  CALL_NAME_STAR,
  CALL_ELLIPSIS,
  IMPORT_IDENTIFIER,
  IMPORT_FN,
  IMPORT_STAR,
  FN_LLVM,
  FN_LAST_KWARG,
  FN_MULTIPLE_ARGS,
  FN_DEFAULT_STARARG,
  FN_ARG_TWICE,
  FN_DEFAULT,
  FN_C_DEFAULT,
  FN_C_TYPE,
  FN_SINGLE_DECORATOR,
  CLASS_EXTENSION,
  CLASS_MISSING_TYPE,
  CLASS_ARG_TWICE,
  CLASS_BAD_DECORATOR,
  CLASS_MULTIPLE_DECORATORS,
  CLASS_SINGLE_DECORATOR,
  CLASS_CONFLICT_DECORATOR,
  CLASS_NONSTATIC_DECORATOR,
  CLASS_BAD_DECORATOR_ARG,
  ID_NOT_FOUND,
  ID_CANNOT_CAPTURE,
  ID_INVALID_BIND,
  UNION_TOO_BIG,
  COMPILER_NO_FILE,
  COMPILER_NO_STDLIB,
  ID_NONLOCAL,
  IMPORT_NO_MODULE,
  IMPORT_NO_NAME,
  DEL_NOT_ALLOWED,
  DEL_INVALID,
  ASSIGN_INVALID,
  ASSIGN_LOCAL_REFERENCE,
  ASSIGN_MULTI_STAR,
  INT_RANGE,
  FLOAT_RANGE,
  STR_FSTRING_BALANCE_EXTRA,
  STR_FSTRING_BALANCE_MISSING,
  CALL_NO_TYPE,
  CALL_TUPLE_COMPREHENSION,
  CALL_NAMEDTUPLE,
  CALL_PARTIAL,
  EXPECTED_TOPLEVEL,
  CLASS_ID_NOT_FOUND,
  CLASS_INVALID_BIND,
  CLASS_NO_INHERIT,
  CLASS_TUPLE_INHERIT,
  CLASS_BAD_MRO,
  CLASS_BAD_ATTR,
  MATCH_MULTI_ELLIPSIS,
  FN_OUTSIDE_ERROR,
  FN_GLOBAL_ASSIGNED,
  FN_GLOBAL_NOT_FOUND,
  FN_NO_DECORATORS,
  FN_BAD_LLVM,
  FN_REALIZE_BUILTIN,
  EXPECTED_LOOP,
  LOOP_DECORATOR,
  BAD_STATIC_TYPE,
  EXPECTED_TYPE,
  UNEXPECTED_TYPE,
  DOT_NO_ATTR,
  DOT_NO_ATTR_ARGS,
  FN_NO_ATTR_ARGS,
  EXPECTED_STATIC,
  EXPECTED_STATIC_SPECIFIED,
  ASSIGN_UNEXPECTED_STATIC,
  ASSIGN_UNEXPECTED_FROZEN,
  CALL_BAD_UNPACK,
  CALL_BAD_ITER,
  CALL_BAD_KWUNPACK,
  CALL_REPEATED_NAME,
  CALL_RECURSIVE_DEFAULT,
  CALL_SUPERF,
  CALL_SUPER_PARENT,
  CALL_PTR_VAR,
  EXPECTED_TUPLE,
  CALL_REALIZED_FN,
  CALL_ARGS_MANY,
  CALL_ARGS_INVALID,
  CALL_ARGS_MISSING,
  GENERICS_MISMATCH,
  EXPECTED_GENERATOR,
  STATIC_RANGE_BOUNDS,
  TUPLE_RANGE_BOUNDS,
  STATIC_DIV_ZERO,
  SLICE_STEP_ZERO,
  OP_NO_MAGIC,
  INST_CALLABLE_STATIC,
  CATCH_EXCEPTION_TYPE,
  TYPE_CANNOT_REALIZE_ATTR,
  TYPE_UNIFY,
  TYPE_FAILED,
  MAX_REALIZATION,
  CUSTOM,
  __END__
};

class ParserErrorInfo : public llvm::ErrorInfo<ParserErrorInfo> {
  ParserErrors errors;

public:
  static char ID;

  explicit ParserErrorInfo(const ErrorMessage &msg) : errors(msg) {}
  explicit ParserErrorInfo(const std::vector<ErrorMessage> &msgs) : errors(msgs) {}
  explicit ParserErrorInfo(const ParserErrors &errors) : errors(errors) {}

  template <class... TA>
  ParserErrorInfo(error::Error e, const codon::SrcInfo &o = codon::SrcInfo(),
                  const TA &...args) {
    auto msg = Emsg(e, args...);
    errors = ParserErrors(ErrorMessage(msg, o, (int)e));
  }

  const ParserErrors &getErrors() const { return errors; }
  ParserErrors &getErrors() { return errors; }

  void log(llvm::raw_ostream &out) const override {
    for (const auto &trace : errors) {
      for (const auto &msg : trace.getMessages()) {
        auto t = msg.toString();
        out << t << "\n";
      }
    }
  }

  std::error_code convertToErrorCode() const override {
    return llvm::inconvertibleErrorCode();
  }
};

class RuntimeErrorInfo : public llvm::ErrorInfo<RuntimeErrorInfo> {
private:
  std::string output;
  std::string type;
  ErrorMessage message;
  std::vector<std::string> backtrace;

public:
  RuntimeErrorInfo(const std::string &output, const std::string &type,
                   const std::string &msg, const std::string &file = "", int line = 0,
                   int col = 0, std::vector<std::string> backtrace = {})
      : output(output), type(type), message(msg, file, line, col),
        backtrace(std::move(backtrace)) {}

  std::string getOutput() const { return output; }
  std::string getType() const { return type; }
  std::string getMessage() const { return message.getMessage(); }
  std::string getFile() const { return message.getFile(); }
  int getLine() const { return message.getLine(); }
  int getColumn() const { return message.getColumn(); }
  std::vector<std::string> getBacktrace() const { return backtrace; }

  void log(llvm::raw_ostream &out) const override {
    out << type << ": ";
    message.log(out);
  }

  std::error_code convertToErrorCode() const override {
    return llvm::inconvertibleErrorCode();
  }

  static char ID;
};

class PluginErrorInfo : public llvm::ErrorInfo<PluginErrorInfo> {
private:
  std::string message;

public:
  explicit PluginErrorInfo(const std::string &message) : message(message) {}

  std::string getMessage() const { return message; }

  void log(llvm::raw_ostream &out) const override { out << message; }

  std::error_code convertToErrorCode() const override {
    return llvm::inconvertibleErrorCode();
  }

  static char ID;
};

class IOErrorInfo : public llvm::ErrorInfo<IOErrorInfo> {
private:
  std::string message;

public:
  explicit IOErrorInfo(const std::string &message) : message(message) {}

  std::string getMessage() const { return message; }

  void log(llvm::raw_ostream &out) const override { out << message; }

  std::error_code convertToErrorCode() const override {
    return llvm::inconvertibleErrorCode();
  }

  static char ID;
};

template <class... TA> std::string Eformat(const TA &...args) { return ""; }
template <class... TA> std::string Eformat(const char *fmt, const TA &...args) {
  return fmt::format(fmt, args...);
}

template <class... TA> std::string Emsg(Error e, const TA &...args) {
  switch (e) {
  /// Validations
  case Error::CALL_NAME_ORDER:
    return fmt::format("positional argument follows keyword argument");
  case Error::CALL_NAME_STAR:
    return fmt::format("cannot use starred expression here");
  case Error::CALL_ELLIPSIS:
    return fmt::format("multiple ellipsis expressions");
  case Error::IMPORT_IDENTIFIER:
    return fmt::format("expected identifier");
  case Error::IMPORT_FN:
    return fmt::format(
        "function signatures only allowed when importing C or Python functions");
  case Error::IMPORT_STAR:
    return fmt::format("import * only allowed at module level");
  case Error::FN_LLVM:
    return fmt::format("return types required for LLVM and C functions");
  case Error::FN_LAST_KWARG:
    return fmt::format("kwargs must be the last argument");
  case Error::FN_MULTIPLE_ARGS:
    return fmt::format("multiple star arguments provided");
  case Error::FN_DEFAULT_STARARG:
    return fmt::format("star arguments cannot have default values");
  case Error::FN_ARG_TWICE:
    return fmt::format("duplicate argument '{}' in function definition", args...);
  case Error::FN_DEFAULT:
    return fmt::format("non-default argument '{}' follows default argument", args...);
  case Error::FN_C_DEFAULT:
    return fmt::format(
        "argument '{}' within C function definition cannot have default value",
        args...);
  case Error::FN_C_TYPE:
    return fmt::format(
        "argument '{}' within C function definition requires type annotation", args...);
  case Error::FN_SINGLE_DECORATOR:
    return fmt::format("cannot combine '@{}' with other attributes or decorators",
                       args...);
  case Error::CLASS_EXTENSION:
    return fmt::format("class extensions cannot define data attributes and generics or "
                       "inherit other classes");
  case Error::CLASS_MISSING_TYPE:
    return fmt::format("type required for data attribute '{}'", args...);
  case Error::CLASS_ARG_TWICE:
    return fmt::format("duplicate data attribute '{}' in class definition", args...);
  case Error::CLASS_BAD_DECORATOR:
    return fmt::format("unsupported class decorator");
  case Error::CLASS_MULTIPLE_DECORATORS:
    return fmt::format("duplicate decorator '@{}' in class definition", args...);
  case Error::CLASS_SINGLE_DECORATOR:
    return fmt::format("cannot combine '@{}' with other attributes or decorators",
                       args...);
  case Error::CLASS_CONFLICT_DECORATOR:
    return fmt::format("cannot combine '@{}' with '@{}'", args...);
  case Error::CLASS_NONSTATIC_DECORATOR:
    return fmt::format("class decorator arguments must be compile-time static values");
  case Error::CLASS_BAD_DECORATOR_ARG:
    return fmt::format("class decorator got unexpected argument");
  /// Simplification
  case Error::ID_NOT_FOUND:
    return fmt::format("name '{}' is not defined", args...);
  case Error::ID_CANNOT_CAPTURE:
    return fmt::format("name '{}' cannot be captured", args...);
  case Error::ID_NONLOCAL:
    return fmt::format("no binding for nonlocal '{}' found", args...);
  case Error::ID_INVALID_BIND:
    return fmt::format("cannot bind '{}' to global or nonlocal name", args...);
  case Error::IMPORT_NO_MODULE:
    return fmt::format("no module named '{}'", args...);
  case Error::IMPORT_NO_NAME:
    return fmt::format("cannot import name '{}' from '{}'", args...);
  case Error::DEL_NOT_ALLOWED:
    return fmt::format("name '{}' cannot be deleted", args...);
  case Error::DEL_INVALID:
    return fmt::format("cannot delete given expression", args...);
  case Error::ASSIGN_INVALID:
    return fmt::format("cannot assign to given expression");
  case Error::ASSIGN_LOCAL_REFERENCE:
    return fmt::format("local variable '{}' referenced before assignment at {}",
                       args...);
  case Error::ASSIGN_MULTI_STAR:
    return fmt::format("multiple starred expressions in assignment");
  case Error::INT_RANGE:
    return fmt::format("integer '{}' cannot fit into 64-bit integer", args...);
  case Error::FLOAT_RANGE:
    return fmt::format("float '{}' cannot fit into 64-bit float", args...);
  case Error::STR_FSTRING_BALANCE_EXTRA:
    return fmt::format("expecting '}}' in f-string");
  case Error::STR_FSTRING_BALANCE_MISSING:
    return fmt::format("single '}}' is not allowed in f-string");
  case Error::CALL_NO_TYPE:
    return fmt::format("cannot use calls in type signatures", args...);
  case Error::CALL_TUPLE_COMPREHENSION:
    return fmt::format(
        "tuple constructor does not accept nested or conditioned comprehensions",
        args...);
  case Error::CALL_NAMEDTUPLE:
    return fmt::format("namedtuple() takes 2 static arguments", args...);
  case Error::CALL_PARTIAL:
    return fmt::format("partial() takes 1 or more arguments", args...);
  case Error::EXPECTED_TOPLEVEL:
    return fmt::format("{} must be a top-level statement", args...);
  case Error::CLASS_ID_NOT_FOUND:
    // Note that type aliases are not valid class names
    return fmt::format("class name '{}' is not defined", args...);
  case Error::CLASS_INVALID_BIND:
    return fmt::format("cannot bind '{}' to class or function", args...);
  case Error::CLASS_NO_INHERIT:
    return fmt::format("{} classes cannot inherit {} classes", args...);
  case Error::CLASS_TUPLE_INHERIT:
    return fmt::format("reference classes cannot inherit tuple classes");
  case Error::CLASS_BAD_MRO:
    return fmt::format("inconsistent class hierarchy");
  case Error::CLASS_BAD_ATTR:
    return fmt::format("unexpected expression in class definition");
  case Error::MATCH_MULTI_ELLIPSIS:
    return fmt::format("multiple ellipses in a pattern");
  case Error::FN_OUTSIDE_ERROR:
    return fmt::format("'{}' outside function", args...);
  case Error::FN_GLOBAL_ASSIGNED:
    return fmt::format("name '{}' is assigned to before global declaration", args...);
  case Error::FN_GLOBAL_NOT_FOUND:
    return fmt::format("no binding for {} '{}' found", args...);
  case Error::FN_NO_DECORATORS:
    return fmt::format("class methods cannot be decorated", args...);
  case Error::FN_BAD_LLVM:
    return fmt::format("invalid LLVM code");
  case Error::FN_REALIZE_BUILTIN:
    return fmt::format("builtin, exported and external functions cannot be generic");
  case Error::EXPECTED_LOOP:
    return fmt::format("'{}' outside loop", args...);
  case Error::LOOP_DECORATOR:
    return fmt::format("invalid loop decorator");
  case Error::BAD_STATIC_TYPE:
    return fmt::format("expected 'int', 'bool' or 'str'");
  case Error::EXPECTED_TYPE:
    return fmt::format("expected {} expression", args...);
  case Error::UNEXPECTED_TYPE:
    return fmt::format("unexpected {} expression", args...);

  /// Typechecking
  case Error::UNION_TOO_BIG:
    return fmt::format(
        "union exceeded its maximum capacity (contains more than {} types)", args...);
  case Error::DOT_NO_ATTR:
    return fmt::format("'{}' object has no attribute '{}'", args...);
  case Error::DOT_NO_ATTR_ARGS:
    return fmt::format("'{}' object has no method '{}' with arguments {}", args...);
  case Error::FN_NO_ATTR_ARGS:
    return fmt::format("no function '{}' with arguments {}", args...);
  case Error::EXPECTED_STATIC:
    return fmt::format("expected static expression");
  case Error::EXPECTED_STATIC_SPECIFIED:
    return fmt::format("expected static {} expression", args...);
  case Error::ASSIGN_UNEXPECTED_STATIC:
    return fmt::format("cannot modify static expressions");
  case Error::ASSIGN_UNEXPECTED_FROZEN:
    return fmt::format("cannot modify tuple attributes");
  case Error::CALL_BAD_UNPACK:
    return fmt::format("argument after * must be a tuple, not '{}'", args...);
  case Error::CALL_BAD_ITER:
    return fmt::format("iterable must be a tuple, not '{}'", args...);
  case Error::CALL_BAD_KWUNPACK:
    return fmt::format("argument after ** must be a named tuple, not '{}'", args...);
  case Error::CALL_REPEATED_NAME:
    return fmt::format("keyword argument repeated: {}", args...);
  case Error::CALL_RECURSIVE_DEFAULT:
    return fmt::format("argument '{}' has recursive default value", args...);
  case Error::CALL_SUPERF:
    return fmt::format("no superf methods found");
  case Error::CALL_SUPER_PARENT:
    return fmt::format("no super methods found");
  case Error::CALL_PTR_VAR:
    return fmt::format("__ptr__() only takes identifiers as arguments");
  case Error::EXPECTED_TUPLE:
    return fmt::format("expected tuple type");
  case Error::CALL_REALIZED_FN:
    return fmt::format("static.realized() only takes functions as a first argument");
  case Error::CALL_ARGS_MANY:
    return fmt::format("{}() takes {} arguments ({} given)", args...);
  case Error::CALL_ARGS_INVALID:
    return fmt::format("'{}' is an invalid keyword argument for {}()", args...);
  case Error::CALL_ARGS_MISSING:
    return fmt::format("{}() missing 1 required positional argument: '{}'", args...);
  case Error::GENERICS_MISMATCH:
    return fmt::format("{} takes {} generics ({} given)", args...);
  case Error::EXPECTED_GENERATOR:
    return fmt::format("expected iterable expression");
  case Error::STATIC_RANGE_BOUNDS:
    return fmt::format("static.range too large (expected 0..{}, got instead {})",
                       args...);
  case Error::TUPLE_RANGE_BOUNDS:
    return fmt::format("tuple index out of range (expected 0..{}, got instead {})",
                       args...);
  case Error::STATIC_DIV_ZERO:
    return fmt::format("static division by zero");
  case Error::SLICE_STEP_ZERO:
    return fmt::format("slice step cannot be zero");
  case Error::OP_NO_MAGIC:
    return fmt::format("unsupported operand type(s) for {}: '{}' and '{}'", args...);
  case Error::INST_CALLABLE_STATIC:
    return fmt::format("CallableTrait cannot take static types");
  case Error::CATCH_EXCEPTION_TYPE:
    return fmt::format("'{}' does not inherit from BaseException", args...);

  case Error::TYPE_CANNOT_REALIZE_ATTR:
    return fmt::format("type of attribute '{}' of object '{}' cannot be inferred",
                       args...);
  case Error::TYPE_UNIFY:
    return fmt::format("'{}' does not match expected type '{}'", args...);
  case Error::TYPE_FAILED:
    return fmt::format(
        "cannot infer the complete type of an expression (inferred only '{}')",
        args...);

  case Error::COMPILER_NO_FILE:
    return fmt::format("cannot open file '{}' for parsing", args...);
  case Error::COMPILER_NO_STDLIB:
    return fmt::format("cannot locate standard library");
  case Error::MAX_REALIZATION:
    return fmt::format(
        "maximum realization depth reached during the realization of '{}'", args...);
  case Error::CUSTOM:
    return Eformat(args...);
  default:
    assert(false);
  }
}

template <class... TA>
void E(Error e, const codon::SrcInfo &o = codon::SrcInfo(), const TA &...args) {
  auto msg = Emsg(e, args...);
  auto err = ParserErrors(ErrorMessage(msg, o, (int)e));
  throw exc::ParserException(err);
}

void E(llvm::Error &&error);

} // namespace error
} // namespace codon
