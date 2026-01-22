// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

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
#define STDLIB_INTERNAL_MODULE "internal"

#define MAIN_IMPORT ""
#define STDLIB_IMPORT ":stdlib:"

#define TYPE_CALLABLE "Callable"
#define TYPE_FUNCTION "Function"
#define TYPE_OPTIONAL "Optional"
#define TYPE_TUPLE "Tuple"
#define TYPE_TYPE "type"

#define TRAIT_TYPE "TypeTrait"
#define TRAIT_CALLABLE "CallableTrait"

#define FN_DISPATCH_SUFFIX ":dispatch"
#define FN_SETTER_SUFFIX ":set_"

#define VAR_CLASS_TOPLEVEL ":toplevel"
#define VAR_USED_SUFFIX ":used"

#define MAX_ERRORS 5
#define MAX_TUPLE 2048
#define MAX_INT_WIDTH 10000
#define MAX_REALIZATION_DEPTH 200
#define MAX_STATIC_ITER 1024

namespace codon {
class Compiler;
}

namespace codon::ast {

extern const std::string VAR_ARGV;
extern const std::string FN_OPTIONAL_UNWRAP;

/// Forward declarations
struct TypeContext;
struct TranslateContext;

/**
 * Cache encapsulation that holds data structures shared across various transformation
 * stages (AST transformation, type checking etc.). The subsequent stages (e.g. type
 * checking) assumes that previous stages populated this structure correctly.
 * Implemented to avoid a bunch of global objects.
 */
struct Cache {
  /// Filesystem object used for accessing files.
  std::shared_ptr<IFilesystem> fs;
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
    /// Set if loaded at toplevel
    bool loadedAtToplevel = true;

    void update(const std::string &name, const std::string &filename,
                const std::shared_ptr<TypeContext> &ctx) {
      this->name = name;
      this->filename = filename;
      this->ctx = ctx;
    }
  };

  /// Compiler
  Compiler *compiler = nullptr;
  /// IR module.
  ir::Module *module = nullptr;

  /// Table of imported files that maps an absolute filename to an Import structure.
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
    ClassStmt *ast = nullptr;
    /// Non-simplified AST. Used for base class instantiation.
    ClassStmt *originalAst = nullptr;

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
      Expr *typeExpr;

      ClassField(const std::string &name, const types::TypePtr &type,
                 const std::string &baseClass, Expr *typeExpr = nullptr)
          : name(name), type(type), baseClass(baseClass), typeExpr(typeExpr) {}
      types::Type *getType() const { return type.get(); }
    };
    /// A list of class' ClassField instances. List is needed (instead of map) because
    /// the order of the fields matters.
    std::vector<ClassField> fields;

    /// Dictionary of class variables: a name maps to a canonical name.
    std::unordered_map<std::string, std::string> classVars;

    /// A class realization.
    struct ClassRealization {
      /// Realized class type.
      std::shared_ptr<types::ClassType> type;
      /// A list of field names and realization's realized field types.
      std::vector<std::pair<std::string, types::TypePtr>> fields;
      /// IR type pointer.
      codon::ir::types::Type *ir = nullptr;
      // Bases (in MRO order)
      std::vector<std::shared_ptr<types::ClassType>> bases;

      /// Realization vtable (for each base class).
      /// Maps {base, function signature} to {thunk realization, thunk ID}.
      /// Base can be the realization itself.
      /// Order is important so map is used instead of unordered_map.
      std::map<std::pair<std::string, std::string>, std::shared_ptr<types::FuncType>>
          vtable;
      /// Realization ID
      size_t id = 0;

      types::ClassType *getType() const { return type.get(); }
    };
    /// Realization lookup table that maps a realized class name to the corresponding
    /// ClassRealization instance.
    std::unordered_map<std::string, std::shared_ptr<ClassRealization>> realizations;

    /// Set if a class is polymorphic and has RTTI.
    bool rtti = false;
    /// List of virtual method names
    std::unordered_set<std::string> virtuals;
    /// MRO
    std::vector<std::shared_ptr<types::ClassType>> mro;

    /// List of statically inherited classes.
    std::vector<std::string> staticParentClasses;

    int jitCell = 0;

    bool hasRTTI() const { return rtti; }
  };
  /// Class lookup table that maps a canonical class identifier to the corresponding
  /// Class instance.
  std::unordered_map<std::string, Class> classes;
  size_t classRealizationCnt = 0;

  Class *getClass(const types::ClassType *);

  std::map<std::pair<std::string, std::string>, size_t> thunkIds;

  struct Function {
    /// Module information
    std::string module;
    std::string rootName;
    /// Generic (unrealized) function template AST.
    FunctionStmt *ast;
    /// Unrealized function type.
    std::shared_ptr<types::FuncType> type;

    /// Non-simplified AST.
    FunctionStmt *origAst = nullptr;
    bool isToplevel = false;

    /// A function realization.
    struct FunctionRealization {
      /// Realized function type.
      std::shared_ptr<types::FuncType> type;
      /// Realized function AST (stored here for later realization in code generations
      /// stage).
      FunctionStmt *ast;
      /// IR function pointer.
      ir::Func *ir;
      /// Resolved captures
      std::vector<std::string> captures;

      types::FuncType *getType() const { return type.get(); }
    };
    /// Realization lookup table that maps a realized function name to the corresponding
    /// FunctionRealization instance.
    std::unordered_map<std::string, std::shared_ptr<FunctionRealization>> realizations =
        {};
    std::set<std::string> captures = {};

    types::FuncType *getType() const { return type.get(); }
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

  /// Custom operators
  std::unordered_map<std::string,
                     std::pair<bool, std::function<Stmt *(ast::TypecheckVisitor *,
                                                          ast::CustomStmt *)>>>
      customBlockStmts;
  std::unordered_map<std::string,
                     std::function<Stmt *(ast::TypecheckVisitor *, ast::CustomStmt *)>>
      customExprStmts;

  /// Set if the Codon is running in JIT mode.
  bool isJit = false;
  int jitCell = 0;

  std::unordered_set<size_t> generatedTuples;
  std::unordered_map<std::string, int> generatedKwTuples;
  std::vector<std::vector<std::string>> generatedTupleNames = {{}};
  ParserErrors errors;

  /// Set if Codon operates in Python compatibility mode (e.g., with Python numerics)
  bool pythonCompat = false;
  /// Set if Codon operates in Python extension mode
  bool pythonExt = false;

public:
  explicit Cache(std::string argv0 = "",
                 const std::shared_ptr<IFilesystem> &fs = nullptr);

  /// Return a uniquely named temporary variable of a format
  /// "{sigil}_{prefix}{counter}". A sigil should be a non-lexable symbol.
  std::string getTemporaryVar(const std::string &prefix = "", char sigil = '%');
  /// Get the non-canonical version of a canonical name.
  std::string rev(const std::string &s);

  /// Generate a unique SrcInfo for internally generated AST nodes.
  SrcInfo generateSrcInfo();
  /// Get file contents at the given location.
  std::string getContent(const SrcInfo &info);

  /// Realization API.

  /// Find a class with a given canonical name and return a matching types::Type pointer
  /// or a nullptr if a class is not found.
  /// Returns an _uninstantiated_ type.
  types::ClassType *findClass(const std::string &name) const;
  /// Find a function with a given canonical name and return a matching types::Type
  /// pointer or a nullptr if a function is not found.
  /// Returns an _uninstantiated_ type.
  types::FuncType *findFunction(const std::string &name) const;
  /// Find the canonical name of a class method.
  std::string getMethod(types::ClassType *typ, const std::string &member);
  /// Find the class method in a given class type that best matches the given arguments.
  /// Returns an _uninstantiated_ type.
  types::FuncType *findMethod(types::ClassType *typ, const std::string &member,
                              const std::vector<types::Type *> &args) const;

  /// Given a class type and the matching generic vector, instantiate the type and
  /// realize it.
  ir::types::Type *realizeType(types::ClassType *type,
                               const std::vector<types::TypePtr> &generics = {});
  /// Given a function type and function arguments, instantiate the type and
  /// realize it. The first argument is the function return type.
  /// You can also pass function generics if a function has one (e.g. T in def
  /// foo[T](...)). If a generic is used as an argument, it will be auto-deduced. Pass
  /// only if a generic cannot be deduced from the provided args.
  ir::Func *realizeFunction(types::FuncType *type,
                            const std::vector<types::TypePtr> &args,
                            const std::vector<types::TypePtr> &generics = {},
                            types::ClassType *parentClass = nullptr);

  ir::types::Type *makeTuple(const std::vector<types::TypePtr> &types);
  ir::types::Type *makeFunction(const std::vector<types::TypePtr> &types);
  ir::types::Type *makeUnion(const std::vector<types::TypePtr> &types);

  size_t getRealizationId(types::ClassType *type);
  std::vector<size_t> getBaseRealizationIds(types::ClassType *type);
  std::vector<size_t> getChildRealizationIds(types::ClassType *type);

  void parseCode(const std::string &code);

  static std::vector<std::shared_ptr<types::ClassType>>
  mergeC3(std::vector<std::vector<types::TypePtr>> &);

  std::shared_ptr<ir::PyModule> pyModule = nullptr;
  void populatePythonModule();

private:
  std::vector<std::unique_ptr<ast::ASTNode>> *_nodes;

public:
  /// Convenience method that constructs a node with the visitor's source location.
  template <typename Tn, typename... Ts> Tn *N(Ts &&...args) {
    _nodes->emplace_back(std::make_unique<Tn>(std::forward<Ts>(args)...));
    Tn *t = static_cast<Tn *>(_nodes->back().get());
    t->cache = this;
    return t;
  }
  template <typename Tn, typename... Ts> Tn *NS(const ASTNode *srcInfo, Ts &&...args) {
    _nodes->emplace_back(std::make_unique<Tn>(std::forward<Ts>(args)...));
    Tn *t = static_cast<Tn *>(_nodes->back().get());
    t->cache = this;
    t->setSrcInfo(srcInfo->getSrcInfo());
    return t;
  }

  std::unordered_map<std::string, double> _timings;
  struct CTimer {
    Cache *c;
    Timer t;
    std::string name;
    CTimer(Cache *c, std::string n) : c(c), t(Timer("")), name(std::move(n)) {}
    double elapsed() const { return t.elapsed(); }
    ~CTimer() {
      c->_timings[name] += t.elapsed();
      t.logged = true;
    }
  };

  template <typename T>
  std::vector<T *> castVectorPtr(std::vector<std::shared_ptr<T>> v) {
    std::vector<T *> r;
    r.reserve(v.size());
    for (const auto &i : v)
      r.emplace_back(i.get());
    return r;
  }
};

} // namespace codon::ast
