// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "codon/cir/cir.h"
#include "codon/cir/pyextension.h"
#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/ctx.h"

#define FILE_GENERATED "<generated>"
#define MODULE_MAIN "__main__"
#define MAIN_IMPORT ""
#define STDLIB_IMPORT ":stdlib:"
#define STDLIB_INTERNAL_MODULE "internal"

#define TYPE_TUPLE "Tuple.N"
#define TYPE_KWTUPLE "KwTuple.N"
#define TYPE_TYPEVAR "TypeVar"
#define TYPE_CALLABLE "Callable"
#define TYPE_PARTIAL "Partial.N"
#define TYPE_OPTIONAL "Optional"
#define TYPE_SLICE "std.internal.types.slice.Slice"
#define FN_UNWRAP "std.internal.types.optional.unwrap.0"
#define VAR_ARGV "__argv__"

#define MAX_INT_WIDTH 10000
#define MAX_REALIZATION_DEPTH 200
#define MAX_STATIC_ITER 1024

namespace codon::ast {

/// Forward declarations
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
  std::unordered_map<std::string, int> identifierCount;
  /// Maps a unique identifier back to the original name in the code
  /// (e.g. Foo.2 -> Foo).
  std::unordered_map<std::string, std::string> reverseIdentifierLookup;
  /// Number of code-generated source code positions. Used to generate the next unique
  /// source-code position information.
  int generatedSrcInfoCount = 0;
  /// Number of unbound variables so far. Used to generate the next unique unbound
  /// identifier.
  int unboundCount = 256;
  /// Number of auto-generated variables so far. Used to generate the next unique
  /// variable name in getTemporaryVar() below.
  int varCount = 0;
  /// Scope counter. Each conditional block gets a new scope ID.
  int blockCount = 1;

  /// Holds module import data.
  struct Module {
    /// Relative module name (e.g., `foo.bar`)
    std::string name;
    /// Absolute filename of an import.
    std::string filename;
    /// Import typechecking context.
    std::shared_ptr<TypeContext> ctx;
    /// Unique import variable for checking already loaded imports.
    std::string importVar;
    /// File content (line:col indexable)
    std::vector<std::string> content;
  };

  /// Absolute path of seqc executable (if available).
  std::string argv0;
  /// Absolute path of the entry-point module (if available).
  std::string module0;
  /// IR module.
  ir::Module *module = nullptr;

  /// Table of imported files that maps an absolute filename to a Import structure.
  /// By convention, the key of the Codon's standard library is ":stdlib:",
  /// and the main module is "".
  std::unordered_map<std::string, Module> imports;

  /// Set of unique (canonical) global identifiers for marking such variables as global
  /// in code-generation step and in JIT.
  std::map<std::string, ir::Var *> globals;

  /// Stores class data for each class (type) in the source code.
  struct Class {
    /// Module information
    std::string module;

    /// Generic (unrealized) class template AST.
    std::shared_ptr<ClassStmt> ast = nullptr;
    /// Non-simplified AST. Used for base class instantiation.
    std::shared_ptr<ClassStmt> originalAst = nullptr;

    /// Class method lookup table. Each non-canonical name points
    /// to a root function name of a corresponding method.
    std::unordered_map<std::string, std::string> methods;

    /// A class field (member).
    struct ClassField {
      /// Field name.
      std::string name;
      /// A corresponding generic field type.
      types::TypePtr type;
      /// Base class name (if available)
      std::string baseClass;
    };
    /// A list of class' ClassField instances. List is needed (instead of map) because
    /// the order of the fields matters.
    std::vector<ClassField> fields;

    /// Dictionary of class variables: a name maps to a canonical name.
    std::unordered_map<std::string, std::string> classVars;

    /// A class realization.
    struct ClassRealization {
      /// Realized class type.
      types::ClassTypePtr type;
      /// A list of field names and realization's realized field types.
      std::vector<std::pair<std::string, types::TypePtr>> fields;
      /// IR type pointer.
      codon::ir::types::Type *ir = nullptr;

      /// Realization vtable.
      struct VTable {
        // Maps {base, thunk signature} to {thunk realization, thunk ID}
        std::map<std::pair<std::string, std::string>,
                 std::pair<types::FuncTypePtr, size_t>>
            table;
        codon::ir::Var *ir = nullptr;
      };
      /// All vtables (for each base class)
      std::unordered_map<std::string, VTable> vtables;
      /// Realization ID
      size_t id = 0;
    };
    /// Realization lookup table that maps a realized class name to the corresponding
    /// ClassRealization instance.
    std::unordered_map<std::string, std::shared_ptr<ClassRealization>> realizations;

    /// Set if a class is polymorphic and has RTTI.
    bool rtti = false;
    /// List of virtual method names
    std::unordered_set<std::string> virtuals;
    /// MRO
    std::vector<ExprPtr> mro;

    /// List of statically inherited classes.
    std::vector<std::string> staticParentClasses;

    bool hasRTTI() const { return rtti; }
  };
  /// Class lookup table that maps a canonical class identifier to the corresponding
  /// Class instance.
  std::unordered_map<std::string, Class> classes;
  size_t classRealizationCnt = 0;

  struct Function {
    /// Module information
    std::string module;
    /// Generic (unrealized) function template AST.
    std::shared_ptr<FunctionStmt> ast = nullptr;
    /// Non-simplified AST.
    std::shared_ptr<FunctionStmt> origAst = nullptr;

    /// A function realization.
    struct FunctionRealization {
      /// Realized function type.
      types::FuncTypePtr type;
      /// Realized function AST (stored here for later realization in code generations
      /// stage).
      std::shared_ptr<FunctionStmt> ast;
      /// IR function pointer.
      ir::Func *ir;
    };
    /// Realization lookup table that maps a realized function name to the corresponding
    /// FunctionRealization instance.
    std::unordered_map<std::string, std::shared_ptr<FunctionRealization>> realizations;

    /// Unrealized function type.
    types::FuncTypePtr type = nullptr;

    std::string rootName;
    bool isToplevel = false;
  };
  /// Function lookup table that maps a canonical function identifier to the
  /// corresponding Function instance.
  std::unordered_map<std::string, Function> functions;

  /// Maps a "root" name of each function to the list of names of the function
  /// overloads (canonical names).
  std::unordered_map<std::string, std::vector<std::string>> overloads;

  /// Pointer to the later contexts needed for IR API access.
  std::shared_ptr<TypeContext> typeCtx = nullptr;
  std::shared_ptr<TranslateContext> codegenCtx = nullptr;
  /// Set of function realizations that are to be translated to IR.
  std::set<std::pair<std::string, std::string>> pendingRealizations;
  /// Mapping of partial record names to function pointers and corresponding masks.
  std::unordered_map<std::string, std::pair<types::FuncTypePtr, std::vector<char>>>
      partials;

  /// Custom operators
  std::unordered_map<std::string,
                     std::pair<bool, std::function<StmtPtr(ast::TypecheckVisitor *,
                                                           ast::CustomStmt *)>>>
      customBlockStmts;
  std::unordered_map<std::string,
                     std::function<StmtPtr(ast::TypecheckVisitor *, ast::CustomStmt *)>>
      customExprStmts;

  /// Plugin-added import paths
  std::vector<std::string> pluginImportPaths;

  /// Set if the Codon is running in JIT mode.
  bool isJit = false;
  int jitCell = 0;

  std::unordered_map<std::string, int> generatedTuples;
  std::vector<exc::ParserException> errors;

  /// Set if Codon operates in Python compatibility mode (e.g., with Python numerics)
  bool pythonCompat = false;
  /// Set if Codon operates in Python extension mode
  bool pythonExt = false;

public:
  explicit Cache(std::string argv0 = "");

  /// Return a uniquely named temporary variable of a format
  /// "{sigil}_{prefix}{counter}". A sigil should be a non-lexable symbol.
  std::string getTemporaryVar(const std::string &prefix = "", char sigil = '%');
  /// Get the non-canonical version of a canonical name.
  std::string rev(const std::string &s);

  /// Generate a unique SrcInfo for internally generated AST nodes.
  SrcInfo generateSrcInfo();
  /// Get file contents at the given location.
  std::string getContent(const SrcInfo &info);
  /// Register a global identifier.
  void addGlobal(const std::string &name, ir::Var *var = nullptr);

  /// Realization API.

  /// Find a class with a given canonical name and return a matching types::Type pointer
  /// or a nullptr if a class is not found.
  /// Returns an _uninstantiated_ type.
  types::ClassTypePtr findClass(const std::string &name) const;
  /// Find a function with a given canonical name and return a matching types::Type
  /// pointer or a nullptr if a function is not found.
  /// Returns an _uninstantiated_ type.
  types::FuncTypePtr findFunction(const std::string &name) const;
  /// Find the canonical name of a class method.
  std::string getMethod(const types::ClassTypePtr &typ, const std::string &member) {
    if (auto m = in(classes, typ->name)) {
      if (auto t = in(m->methods, member))
        return *t;
    }
    seqassertn(false, "cannot find '{}' in '{}'", member, typ->toString());
    return "";
  }
  /// Find the class method in a given class type that best matches the given arguments.
  /// Returns an _uninstantiated_ type.
  types::FuncTypePtr findMethod(types::ClassType *typ, const std::string &member,
                                const std::vector<types::TypePtr> &args);

  /// Given a class type and the matching generic vector, instantiate the type and
  /// realize it.
  ir::types::Type *realizeType(types::ClassTypePtr type,
                               const std::vector<types::TypePtr> &generics = {});
  /// Given a function type and function arguments, instantiate the type and
  /// realize it. The first argument is the function return type.
  /// You can also pass function generics if a function has one (e.g. T in def
  /// foo[T](...)). If a generic is used as an argument, it will be auto-deduced. Pass
  /// only if a generic cannot be deduced from the provided args.
  ir::Func *realizeFunction(types::FuncTypePtr type,
                            const std::vector<types::TypePtr> &args,
                            const std::vector<types::TypePtr> &generics = {},
                            const types::ClassTypePtr &parentClass = nullptr);

  ir::types::Type *makeTuple(const std::vector<types::TypePtr> &types);
  ir::types::Type *makeFunction(const std::vector<types::TypePtr> &types);
  ir::types::Type *makeUnion(const std::vector<types::TypePtr> &types);

  void parseCode(const std::string &code);

  static std::vector<ExprPtr> mergeC3(std::vector<std::vector<ExprPtr>> &);

  std::shared_ptr<ir::PyModule> pyModule = nullptr;
  void populatePythonModule();
};

} // namespace codon::ast
