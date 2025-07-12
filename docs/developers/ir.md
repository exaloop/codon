After type checking but before native code generation, the Codon compiler
makes use of a new [intermediate representation](https://en.wikipedia.org/wiki/Intermediate_representation)
called CIR, where a number of higher-level optimizations, transformations and analyses take place.
CIR offers a comprehensive framework for writing new optimizations or
analyses without having to deal with cumbersome abstract syntax trees (ASTs)
albeit while preserving higher-level program constructs and semantics.

## Motivation

In Codon's optimization framework, we often found the need to reason about higher-level
Python constructs, which wasn't feasible through LLVM IR. For instance, one optimization
Codon performs pertains to removing redundant dictionary accesses, e.g. as found in this
common code pattern:

``` python
d[k] = d.get(k, 0) + 1  # increment count of key 'k' in dict 'd'
```

The original code performs two lookups of key `k`, but only one lookup is required. To
address this, we want to find patterns of the form `d[k] = d.get(k, i) + j` and replace them
with an implementation that performs just one lookup of `k`. It turns out that this is nearly
impossible to do at the LLVM IR level as `dict.__setitem__()` (which corresponds to the
assignment `d[k] = ...`) and `dict.get()` both compile to large, complex LLVM IR that we'd
have no hope of recognizing and making sense of after the fact — in other words, the semantic
meaning of the original code (i.e. "increment the count of `k` in `d`") is effectively lost.

To solve this problem, Codon employs its own IR, which sits between the abstract syntax tree and
LLVM IR in the compilation process. Codon IR is much simpler than the AST, but still retains
enough information to reason about semantics and higher-level constructs. Identifying the
dictionary pattern above, for example, amounts to simply identifying a short sequence of IR
that resembles `dict.__setitem__(d, k, dict.get(d, k, i) + j)`.

## At a glance

Here is a small (simplified) example showcasing CIR in action. Consider the code:

``` python
def fib(n):
    if n < 2:
        return 1
    else:
        return fib(n - 1) + fib(n - 2)
```

When instantiated with an `int` argument, the following IR gets produced (the
names have been cleaned up for simplicity):

``` lisp
(bodied_func
  '"fib[int]"
  (type '"fib[int]")
  (args (var '"n" (type '"int") (global false)))
  (vars)
  (series
    (if (call '"int.__lt__[int,int]" '"n" 2)
      (series (return 1))
      (series
        (return
          (call
            '"int.__add__[int,int]"
            (call
              '"fib[int]"
              (call '"int.__sub__[int,int]" '"n" 1))
            (call
              '"fib[int]"
              (call '"int.__sub__[int,int]" '"n" 2))))))))
```

A few interesting points to consider:

- CIR is hierarchical like ASTS, but unlike ASTs it uses a vastly reduced
  set of nodes, making it much easier to work with and reason about.
- Operators are expressed as function calls. In fact, CIR has no explicit
  concept of `+`, `-`, etc. and instead expresses these via their corresponding
  magic methods (`__add__`, `__sub__`, etc.).
- CIR has no concept of generic types. By the time CIR is generated, all types
  need to have been resolved.

## Structure

CIR is comprised of a set of *nodes*, each with a specific semantic meaning.
There are nodes for representing constants (e.g. `42`), instructions (e.g. `call`)
control flow (e.g. `if`), types (e.g. `int`) and so on.

Here is the node hierarchy:

``` mermaid
graph TD
  N(Node) --> M(Module);
  N --> T(Type);
  N --> VR(Var);
  N --> VL(Value);
  VR --> FN(Func);
  VL --> C(Const);
  VL --> I(Instr);
  VL --> FL(Flow);
```

Here is a breakdown of the different types of nodes:

- **Module**: a special container node that represents the entire program, i.e. all
  the functions needed by the program as well as the main code that is executed when
  running the program (in practice, we simply put this code into a new implicit "main"
  function).
- **Type**: a kind of node that is attached to all other nodes and represents the given
  node's data type, be it an integer, float, string etc. One important aspect of Codon IR
  is that it's fully typed, meaning we never deal with ambiguous or unknown types; type
  checking is done on the AST before Codon IR is generated.
- **Var**: represents a variable in a program. Variables are usually produced from assignments
  like `x = y`, but they can also be produced from control-flow statements like
  `for i in range(10)`, which creates variable `i`.
- **Func**: a type of **Var** that represents a function.
- **Value**: a generic node that represents values, which can be anything from constants to
  variable references or function calls and so on.
- **Instr**: a type of Value that represents a specific operation, such as a function call,
  return statement, conditional expression (i.e. `x if y else z`) and so on.
- **Flow**: represents control-flow statements like `if`-statements, `while`-loops,
  `try`-`except` etc. These are first-class values in Codon IR.
- **Const**: represents a constant like `42`, `"hello"` or `3.14`.

## Uses

CIR provides a framework for doing program optimizations, analyses and transformations.
These operations are collectively known as IR *passes*.

A number of built-in passes and other functionalities are provided by CIR. These can be
used as building blocks to create new passes. Examples include:

- Control-flow graph creation
- Reaching definitions
- Dominator analysis
- Side effect analysis
- Constant propagation and folding
- Canonicalization
- Inlining and outlining
- Python-specific optimizations targeting several common Python idioms

We're regularly adding new standard passes, so this list is always growing.

### An example

Let's look at a real example. Imagine we want to write a pass that transforms expressions
of the form `<int const> + <int const>` into a single `<int const>` denoting the result.
In other words, a simple form of constant folding that only looks at addition on integers.
The resulting pass would like this:

``` cpp
#include "codon/cir/transform/pass.h"
#include "codon/cir/util/irtools.h"

using namespace codon::ir;

class MyAddFolder : public transform::OperatorPass {
public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  void handle(CallInstr *v) override {
    auto *f = util::getFunc(v->getCallee());
    if (!f || f->getUnmangledName() != "__add__" || v->numArgs() != 2)
        return;

    auto *lhs = cast<IntConst>(v->front());
    auto *rhs = cast<IntConst>(v->back());

    if (lhs && rhs) {
      long sum = lhs->getVal() + rhs->getVal();
      v->replaceAll(v->getModule()->getInt(sum));
    }
  }
};

const std::string MyAddFolder::KEY = "my-add-folder";
```

So how does this actually work, and what do the different components mean? Here
are some notable points:

- Most passes can inherit from `transform::OperatorPass`. `OperatorPass` is a combination
  of an `Operator` and a `Pass`. An `Operator` is a utility visitor that provides hooks for
  handling all the different node types (i.e. through the `handle()` methods). `Pass` is the
  base class representing a generic pass, which simply provides a `run()` method that takes
  a module.
- Because of this, `MyAddFolder::handle(CallInstr *)` will be called on every call instruction
  in the module.
- Within our `handle()`, we first check to see if the function being called is `__add__`, indicating
  addition (in practice there would be a more specific check to make sure this is *the* `__add__`),
  and if so we extract the first and second arguments.
- We cast these arguments to `IntConst`. If the results are non-null, then both arguments were in fact
  integer constants, meaning we can replace the original call instruction with a new constant that
  represents the result of the addition. In CIR, all nodes are "replaceable" via a `replaceAll()` method.
- Lastly, notice that all passes have a `KEY` field to uniquely identify them.

### Bidirectionality

An important and often very useful feature of CIR is that it is *bidirectional*, meaning it's possible
to return to the type checking stage to generate new IR nodes that were not initially present in the
module. For example, imagine that your pass needs to use a `List` with some new element type; that list's
methods need to be instantiated by the type checker for use in CIR. In practice this bidirectionality
often lets you write large parts of your optimization or transformation in Codon, and pull out the necessary
functions or types as needed in the pass.

CIR's `Module` class has three methods to enable this feature:

``` cpp
  /// Gets or realizes a function.
  /// @param funcName the function name
  /// @param args the argument types
  /// @param generics the generics
  /// @param module the module of the function
  /// @return the function or nullptr
  Func *getOrRealizeFunc(const std::string &funcName, std::vector<types::Type *> args,
                         std::vector<types::Generic> generics = {},
                         const std::string &module = "");

  /// Gets or realizes a method.
  /// @param parent the parent class
  /// @param methodName the method name
  /// @param rType the return type
  /// @param args the argument types
  /// @param generics the generics
  /// @return the method or nullptr
  Func *getOrRealizeMethod(types::Type *parent, const std::string &methodName,
                           std::vector<types::Type *> args,
                           std::vector<types::Generic> generics = {});

  /// Gets or realizes a type.
  /// @param typeName mangled type name
  /// @param generics the generics
  /// @return the function or nullptr
  types::Type *getOrRealizeType(const std::string &typeName,
                                std::vector<types::Generic> generics = {});
```

Let's see bidirectionality in action. Consider the following Codon code:

``` python
def foo(x):
    return x*3 + x

def validate(x, y):
    assert y == x*4

a = foo(10)
b = foo(1.5)
c = foo('a')
```

Assume we want our pass to insert a call to `validate()` after each assignment that takes the assigned variable
and the argument passed to `foo()`. We would do something like the following:

``` cpp
#include "codon/cir/transform/pass.h"
#include "codon/cir/util/irtools.h"

using namespace codon::ir;

class ValidateFoo : public transform::OperatorPass {
public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  void handle(AssignInstr *v) override {
    auto *M = v->getModule();
    auto *var = v->getLhs();
    auto *call = cast<CallInstr>(v->getRhs());
    if (!call)
      return;

    auto *foo = util::getFunc(call->getCallee());
    if (!foo || foo->getUnmangledName() != "foo")
      return;

    auto *arg1 = call->front();         // argument of 'foo' call
    auto *arg2 = M->Nr<VarValue>(var);  // result of 'foo' call
    auto *validate =
      M->getOrRealizeFunc("validate", {arg1->getType(), arg2->getType()});
    if (!validate)
      return;

    auto *validateCall = util::call(validate, {arg1, arg2});
    insertAfter(validateCall);  // call 'validate' after 'foo'
  }
};

const std::string ValidateFoo::KEY = "validate-foo";
```

Note that `insertAfter` is a convenience method of `Operator` that inserts the given node "after" the node
being visited (along with `insertBefore` which inserts *before* the node being visited).

Running this pass on the snippet above, we would get:

``` python
a = foo(10)
validate(10, a)

b = foo(1.5)
validate(1.5, b)

c = foo('a')
validate('a', c)
```

Notice that we used `getOrRealizeFunc` to create three different instances of `validate`: one for `int`
arguments, one for `float` arguments and finally one for `str` arguments.

## Extending the IR

CIR is extensible, and it is possible to add new constants, instructions, flows and types. This can be
done by subclassing the corresponding *custom* base class; to create a custom type, for example, you
would subclass `CustomType`. Let's look at an example where we extend CIR to add a 32-bit float type
(note that Codon already has a native 32-bit float type; this is just for demonstration purposes):

``` cpp
using namespace codon::ir;

#include "codon/cir/dsl/nodes.h"
#include "codon/cir/llvm/llvisitor.h"

class Builder : public dsl::codegen::TypeBuilder {
public:
  llvm::Type *buildType(LLVMVisitor *v) override {
    return v->getBuilder()->getFloatTy();
  }

  llvm::DIType *buildDebugType(LLVMVisitor *v) override {
    auto *module = v->getModule();
    auto &layout = module->getDataLayout();
    auto &db = v->getDebugInfo();
    auto *t = buildType(v);
    return db.builder->createBasicType(
           "float_32",
           layout.getTypeAllocSizeInBits(t),
           llvm::dwarf::DW_ATE_float);
  }
};

class Float32 : public dsl::CustomType {
public:
  std::unique_ptr<TypeBuilder> getBuilder() const override {
    return std::make_unique<Builder>();
  }
};
```

Notice that, in order to specify how to generate code for our `Float32` type, we create a `TypeBuilder`
subclass with methods for building the corresponding LLVM IR type. There is also a `ValueBuilder` for
new constants and converting them to LLVM IR, as well as a `CFBuilder` for new instructions and creating
control-flow graphs out of them.

!!! tip

    When subclassing nodes other than types (e.g. instructions, flows, etc.), be sure to use the `AcceptorExtend`
    [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) class, as in
    `class MyNewInstr : public AcceptorExtend<MyNewInstr, dsl::CustomInstr>`.


## Utilities

The `codon/cir/util/` directory has a number of utility and generally helpful functions, for things like
cloning IR, inlining/outlining, matching and more. `codon/cir/util/irtools.h` in particular has many helpful
functions for performing various common tasks. If you're working with CIR, be sure to take a look at these
functions to make your life easier!

## Standard pass pipeline

These standard sets of passes are run in `release`-mode:

- Python-specific optimizations: a series of passes to optimize common Python patterns and
  idioms. Examples include dictionary updates of the form `d[k] = d.get(k, x) <op> y`, and
  optimizing them to do just *one* access into the dictionary, as well as optimizing repeated
  string concatenations, various I/O patterns, list operations and so on.

- Imperative `for`-loop lowering: loops of the form `for i in range(a, b, c)` (with `c` constant)
  are lowered to a special IR node, since these loops are important for e.g. multithreading later.

- A series of program analyses whose results are available to later passes:
  - [Control-flow analysis](https://en.wikipedia.org/wiki/Control_flow_analysis)
  - [Reaching definition analysis](https://en.wikipedia.org/wiki/Reaching_definition)
  - [Dominator analysis](https://en.wikipedia.org/wiki/Dominator_(graph_theory))
  - [Capture (or escape) analysis](https://en.wikipedia.org/wiki/Escape_analysis)

- Parallel loop lowering for multithreading or GPU

- Constant propagation and folding. This also includes dead code elimination and (in non-JIT mode)
  global variable demotion.

- Library-specific optimizations, such as operator fusion for [NumPy](/libraries/numpy).

Codon [plugins](/developers/extend) can inject their own passes into the pipeline as well.
