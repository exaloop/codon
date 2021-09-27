/*
 * cache.h --- Code transformation cache (shared objects).
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#pragma once

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"
#include "parser/ctx.h"

#include "sir/sir.h"

#define TYPECHECK_MAX_ITERATIONS 100
#define FILE_GENERATED "<generated>"
#define MODULE_MAIN "__main__"
#define MAIN_IMPORT ""
#define STDLIB_IMPORT ":stdlib:"
#define STDLIB_INTERNAL_MODULE "internal"
#define ATTR_EXTERN_LLVM "llvm"
#define ATTR_EXTEND "extend"

#define TYPE_TUPLE "Tuple.N"
#define TYPE_KWTUPLE "KwTuple.N"
#define TYPE_FUNCTION "Function.N"
#define TYPE_CALLABLE "Callable.N"
#define TYPE_PARTIAL "Partial.N"
#define TYPE_OPTIONAL "Optional"
#define TYPE_EXCHEADER "std.internal.types.error.ExcHeader"
#define TYPE_SLICE "std.internal.types.slice.Slice"
#define FN_UNWRAP "std.internal.types.optional.unwrap"
#define VAR_ARGV "__argv__"

#define FLAG_METHOD 1
#define FLAG_ATOMIC 2
#define FLAG_TEST 4

namespace seq {
namespace ast {

/// Forward declarations
struct SimplifyContext;
class SimplifyVisitor;
struct TypeContext;
struct TranslateContext;

/**
 * Cache encapsulation that holds data structures shared across various transformation
 * stages (AST transformation, type checking etc.). The subsequent stages (e.g. type
 * checking) assumes that previous stages populated this structure correctly.
 * Implemented to avoid bunch of global objects.
 */
struct Cache : public std::enable_shared_from_this<Cache> {
  /// Stores a count for each identifier (name) seen in the code.
  /// Used to generate unique identifier for each name in the code (e.g. Foo -> Foo.2).
  unordered_map<string, int> identifierCount;
  /// Maps a unique identifier back to the original name in the code
  /// (e.g. Foo.2 -> Foo).
  unordered_map<string, string> reverseIdentifierLookup;
  /// Number of code-generated source code positions. Used to generate the next unique
  /// source-code position information.
  int generatedSrcInfoCount;
  /// Number of unbound variables so far. Used to generate the next unique unbound
  /// identifier.
  int unboundCount;
  /// Number of auto-generated variables so far. Used to generate the next unique
  /// variable name in getTemporaryVar() below.
  int varCount;
  /// Stores the count of imported files. Used to track class method ages
  /// and to prevent using extended methods before they were seen.
  int age;
  /// Test flags for seqtest test cases. Zero if seqtest is not parsing the code.
  int testFlags;

  /// Holds module import data.
  struct Import {
    /// Absolute filename of an import.
    string filename;
    /// Import simplify context.
    shared_ptr<SimplifyContext> ctx;
    /// Unique import variable for checking already loaded imports.
    string importVar;
    /// File content (line:col indexable)
    vector<string> content;
  };

  /// Absolute path of seqc executable (if available).
  string argv0;
  /// Absolute path of the entry-point module (if available).
  string module0;
  /// LLVM module.
  seq::ir::Module *module = nullptr;

  /// Table of imported files that maps an absolute filename to a Import structure.
  /// By convention, the key of Seq standard library is "".
  unordered_map<string, Import> imports;

  /// Set of unique (canonical) global identifiers for marking such variables as global
  /// in code-generation step.
  set<string> globals;

  /// Stores class data for each class (type) in the source code.
  struct Class {
    /// Generic (unrealized) class template AST.
    shared_ptr<ClassStmt> ast;
    /// Non-simplified AST. Used for base class instantiation.
    shared_ptr<ClassStmt> originalAst;

    /// A class function method.
    struct ClassMethod {
      /// Canonical name of a method (e.g. __init__.1).
      string name;
      /// A corresponding generic function type.
      types::FuncTypePtr type;
      /// Method age (how many class extension were seen before a method definition).
      /// Used to prevent the usage of a method before it was defined in the code.
      int age;
    };
    /// Class method lookup table. Each name points to a list of ClassMethod instances
    /// that share the same method name (a list because methods can be overloaded).
    unordered_map<string, vector<ClassMethod>> methods;

    /// A class field (member).
    struct ClassField {
      /// Field name.
      string name;
      /// A corresponding generic field type.
      types::TypePtr type;
    };
    /// A list of class' ClassField instances. List is needed (instead of map) because
    /// the order of the fields matters.
    vector<ClassField> fields;

    /// A class realization.
    struct ClassRealization {
      /// Realized class type.
      types::ClassTypePtr type;
      /// A list of field names and realization's realized field types.
      vector<std::pair<string, types::TypePtr>> fields;
      /// IR type pointer.
      seq::ir::types::Type *ir;
    };
    /// Realization lookup table that maps a realized class name to the corresponding
    /// ClassRealization instance.
    unordered_map<string, shared_ptr<ClassRealization>> realizations;

    Class() : ast(nullptr), originalAst(nullptr) {}
  };
  /// Class lookup table that maps a canonical class identifier to the corresponding
  /// Class instance.
  unordered_map<string, Class> classes;

  struct Function {
    /// Generic (unrealized) function template AST.
    shared_ptr<FunctionStmt> ast;

    /// A function realization.
    struct FunctionRealization {
      /// Realized function type.
      types::FuncTypePtr type;
      /// Realized function AST (stored here for later realization in code generations
      /// stage).
      shared_ptr<FunctionStmt> ast;
      /// IR function pointer.
      ir::Func *ir;
    };
    /// Realization lookup table that maps a realized function name to the corresponding
    /// FunctionRealization instance.
    unordered_map<string, shared_ptr<FunctionRealization>> realizations;

    /// Unrealized function type.
    types::FuncTypePtr type;

    Function() : ast(nullptr), type(nullptr) {}
  };
  /// Function lookup table that maps a canonical function identifier to the
  /// corresponding Function instance.
  unordered_map<string, Function> functions;

  /// Pointer to the later contexts needed for IR API access.
  shared_ptr<TypeContext> typeCtx;
  shared_ptr<TranslateContext> codegenCtx;
  /// Set of function realizations that are to be translated to IR.
  set<std::pair<string, string>> pendingRealizations;

  /// Custom operators
  unordered_map<string, std::pair<bool, std::function<StmtPtr(ast::SimplifyVisitor *,
                                                              ast::CustomStmt *)>>>
      customBlockStmts;
  unordered_map<string,
                std::function<StmtPtr(ast::SimplifyVisitor *, ast::CustomStmt *)>>
      customExprStmts;

public:
  explicit Cache(string argv0 = "");

  /// Return a uniquely named temporary variable of a format
  /// "{sigil}_{prefix}{counter}". A sigil should be a non-lexable symbol.
  string getTemporaryVar(const string &prefix = "", char sigil = '.');

  /// Generate a unique SrcInfo for internally generated AST nodes.
  SrcInfo generateSrcInfo();
  /// Get file contents at the given location.
  string getContent(const SrcInfo &info);

  /// Realization API.

  /// Find a class with a given canonical name and return a matching types::Type pointer
  /// or a nullptr if a class is not found.
  /// Returns an _uninstantiated_ type.
  types::ClassTypePtr findClass(const string &name) const;
  /// Find a function with a given canonical name and return a matching types::Type
  /// pointer or a nullptr if a function is not found.
  /// Returns an _uninstantiated_ type.
  types::FuncTypePtr findFunction(const string &name) const;
  /// Find the class method in a given class type that best matches the given arguments.
  /// Returns an _uninstantiated_ type.
  types::FuncTypePtr findMethod(types::ClassType *typ, const string &member,
                                const vector<pair<string, types::TypePtr>> &args);

  /// Given a class type and the matching generic vector, instantiate the type and
  /// realize it.
  ir::types::Type *realizeType(types::ClassTypePtr type,
                               const vector<types::TypePtr> &generics = {});
  /// Given a function type and function arguments, instantiate the type and
  /// realize it. The first argument is the function return type.
  /// You can also pass function generics if a function has one (e.g. T in def
  /// foo[T](...)). If a generic is used as an argument, it will be auto-deduced. Pass
  /// only if a generic cannot be deduced from the provided args.
  ir::Func *realizeFunction(types::FuncTypePtr type, const vector<types::TypePtr> &args,
                            const vector<types::TypePtr> &generics = {},
                            types::ClassTypePtr parentClass = nullptr);

  ir::types::Type *makeTuple(const vector<types::TypePtr> &types);
  ir::types::Type *makeFunction(const vector<types::TypePtr> &types);
};

} // namespace ast
} // namespace seq
