Codon has a JIT that can be embedded in and called from C++ applications.
The Codon distribution contains all the necessary headers and shared
libraries for compiling and linking against the Codon compiler library,
which includes the JIT.

## A minimal example

``` cpp
#include <iostream>

#include "codon/compiler/jit.h"

int main(int argc, char **argv) {
  std::string mode = "";
  std::string stdlib = "/path/to/.codon/lib/codon/stdlib";
  std::string code = "sum(i**2 for i in range(10))";

  codon::jit::JIT jit(argv[0], mode, stdlib);
  llvm::cantFail(jit.init());
  std::cout << llvm::cantFail(jit.execute(code)) << std::endl;
}
```

The `codon::jit::JIT` constructor takes three arguments:

- `argv[0]`: as received by `main()`
- `mode`: currently supports either an empty string or `"jupyter"`, where the
  latter adds support for the
  [`_repr_mimebundle_`](https://ipython.readthedocs.io/en/latest/whatsnew/version5.html#define-repr-mimebundle)
  method.
- `stdlib`: path to Codon standard library; `~/.codon/lib/codon/stdlib` if using
  a standard Codon installation

Next, the `init()` method is used to initialize the JIT. Note that JIT methods make
use of [LLVM's error handling](https://llvm.org/docs/ProgrammersManual.html#error-handling),
hence the use of [`llvm::cantFail()`](https://llvm.org/doxygen/namespacellvm.html#aa1e1474f15df639f0d874b21f15666f7).

Lastly, code can be executed using the `execute()` method, which captures and returns
the output of the provided code string.

The program can be compiled as follows:

``` bash
export CODON_DIR=~/.codon  # or wherever Codon is installed
g++ -std=c++20 -I${CODON_DIR}/include \
               -L${CODON_DIR}/lib/codon \
               -Wl,-rpath,${CODON_DIR}/lib/codon \
               -lcodonc \
               test.cpp
```

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

Compiles the given code string to a [Codon IR](/developers/ir) function representing
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
reinterpret_cast<void (*)()>(p)();  // prints 285
```

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

Memory allocated by Codon during parsing and type checking is automatically released between
JIT invocations. Memory allocated by LLVM during compilation can be controlled via
`llvm::orc::ResourceTracker` as described above.

Codon IR provides a mechanism to release allocated IR nodes via an arena interface:

``` cpp
jit.getCompiler()->getModule()->pushArena();
// ... work with jit ...
jit.getCompiler()->getModule()->popArena();
```

Once an arena is popped, all IR nodes allocated since the corresponding push will be deallocated.
Arenas should almost always be used in "forgetful" mode (i.e. when passing `forgetful=true` to the
`init()` method), since otherwise IR nodes might need to be reused in future JIT inputs.
