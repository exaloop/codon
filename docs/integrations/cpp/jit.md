---
title: Embed Codon JIT
---

Codon has a JIT that can be embedded in and called from C++ applications.
The Codon distribution contains all the necessary headers and shared
libraries for compiling and linking against the Codon compiler library,
which includes the JIT.

## A minimal example

``` cpp
#include <iostream>
#include <utility>

#include "codon/compiler/jit.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

int main(int argc, char **argv) {
  auto options = codon::Options::getDefault(argv[0]);
  options->jit = true;
  options->capture = true;
  std::string mode = "";
  std::string stdlib = "/path/to/.codon/lib/codon/stdlib";
  std::string code = "print(sum(value * value for value in range(10)))";

  codon::jit::JIT jit(*options, mode, stdlib);
  if (auto error = jit.init()) {
    llvm::logAllUnhandledErrors(std::move(error), llvm::errs(), "Codon: ");
    return 1;
  }
  auto output = jit.execute(code);
  if (!output) {
    llvm::logAllUnhandledErrors(output.takeError(), llvm::errs(), "Codon: ");
    return 1;
  }
  std::cout << *output;
  return 0;
}
```

The `codon::jit::JIT` constructor takes three arguments:

- `options`: a `codon::Options` configuration. `getDefault(argv[0])` supplies
  defaults and the executable location used for library discovery; the
  example then enables JIT compilation and output capture.
- `mode`: currently supports either an empty string or `"jupyter"`, where the
  latter adds support for the
  [`_repr_mimebundle_`](https://ipython.readthedocs.io/en/latest/whatsnew/version5.html#define-repr-mimebundle)
  method.
- `stdlib`: path to Codon standard library; `~/.codon/lib/codon/stdlib` if using
  a standard Codon installation

The `init()` method initializes the JIT. Its methods use
[LLVM's error handling](https://llvm.org/docs/ProgrammersManual.html#error-handling),
so the example checks both initialization and execution errors before using
their results. This lets the host report a compilation or execution failure
without assuming every submitted program is valid.

Lastly, code can be executed using the `execute()` method, which captures and returns
the output of the provided code string.

The program can be compiled as follows:

``` bash
export CODON_DIR=~/.codon  # or wherever Codon is installed
c++ -std=c++20 test.cpp -I"${CODON_DIR}/include" \
  -L"${CODON_DIR}/lib/codon" \
  -Wl,-rpath,"${CODON_DIR}/lib/codon" \
  -lcodonc -o jit-example
```

After setting `stdlib` to the installed standard-library directory, running
`./jit-example` prints `285`. The headers and compiler library should come
from the same Codon SDK, since this C++ interface is version-dependent.

## JIT API

`codon::jit::JIT` provides the following methods:

### `init`

``` cpp
llvm::Error init(bool forgetful = false);
```

Initializes the JIT. If `forgetful` is `true`, then the JIT will not remember
global variables or functions defined in previous inputs, effectively resulting
in a fresh JIT on each input (albeit without having to perform initialization
repeatedly).

### `compile` (code string)

``` cpp
llvm::Expected<ir::Func *> compile(const std::string &code,
                                   const std::string &file = "", int line = 0);
```

Compiles the given code string to a [Codon IR](../../developers/ir.md) function representing
the JIT input. Optional file and line information can be passed through the `file` and
`line` arguments, respectively. Does not invoke the LLVM backend.


### `compile` (IR function)

``` cpp
llvm::Error compile(const ir::Func *input,
                    llvm::orc::ResourceTrackerSP rt = nullptr);
```

Compiles the given Codon IR function in the JIT. An optional
[`llvm::orc::ResourceTracker`](https://llvm.org/doxygen/classllvm_1_1orc_1_1ResourceTracker.html)
can be provided to manage the JIT-compiled code. A resource tracker can be created via
`jit.getEngine()->getMainJITDylib().createResourceTracker()`.

### `address`

``` cpp
llvm::Expected<void *> address(const ir::Func *input,
                               llvm::orc::ResourceTrackerSP rt = nullptr);
```

Returns a pointer to the compiled function corresponding to the provided Codon IR function. As
above, An optional `llvm::orc::ResourceTracker` can be provided to manage the JIT-compiled code.
The returned pointer can be cast to the appropriate function pointer and called. For example,
building on the code above, we can do the following:

``` cpp
auto *f = llvm::cantFail(jit.compile(code));
auto *p = llvm::cantFail(jit.address(f));
reinterpret_cast<void (*)()>(p)();  // executes the compiled input
```

The function pointer remains usable only while its JIT and associated code
resources remain alive. Unloading a resource tracker invalidates pointers
into the code it owns.

### `execute`

``` cpp
llvm::Expected<std::string> execute(const std::string &code,
                                    const std::string &file = "", int line = 0,
                                    bool debug = false,
                                    llvm::orc::ResourceTrackerSP rt = nullptr);
```

Runs the full compilation pipeline and executes the given code string. Optional arguments correspond
to those described above. Returns captured output of the code string.

## Controlling memory usage

Stateful JIT sessions retain definitions that later inputs may use, so their
memory use can grow as new types and functions are compiled. Forgetful mode
is useful when inputs are independent and do not need that persistent state.
LLVM code resources can be managed through `llvm::orc::ResourceTracker` as
described above; unloading them also ends the lifetime of their function
pointers.

Codon IR provides a mechanism to release allocated IR nodes via an arena interface:

``` cpp
jit.getCompiler()->getModule()->pushArena();
// ... work with jit ...
jit.getCompiler()->getModule()->popArena();
```

Once an arena is popped, all IR nodes allocated since the corresponding push will be deallocated.
Arenas should almost always be used in "forgetful" mode (i.e. when passing `forgetful=true` to the
`init()` method), since otherwise IR nodes might need to be reused in future JIT inputs.
