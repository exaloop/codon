Codon supports plugins that can be loaded dynamically. Plugins can
contain IR passes that can be inserted into the Codon IR pass pipeline,
as well as LLVM passes to be used during backend code generation.

## Configuration

Plugin information is specified through a [TOML](https://toml.io/en/) file
called `plugin.toml`. The following fields are supported:

- `about.name`: Plugin name
- `about.description`: Plugin description
- `about.version`: Plugin version, using [semantic versioning](https://semver.org)
- `about.url`: Plugin URL
- `about.supported`: Supported Codon versions, using semantic versioning ranges
- `library.cpp`: Legacy shared library, used for both the compiler implementation
  and runtime functions unless overridden below. Existing configurations retain
  their behavior.
- `library.compiler`: Shared library containing the compiler plugin implementation
  and its `load()` entry point. It is loaded into the compiler and is linked into
  generated programs only if also selected as the runtime library. An omitted value
  inherits `library.cpp`; an empty string disables compiler-library loading.
- `library.runtime`: Shared library containing runtime functions used by generated
  code. It is linked into executables and shared libraries, and loaded for execution
  by `codon run` or the JIT. It does not need a `load()` entry point and is not loaded
  during ordinary compilation. An omitted value inherits `library.cpp`; an empty
  string disables automatic runtime linking and runtime-library loading.
  All three library paths are relative to the manifest directory. Their extensions
  (`.so` or `.dylib`) are added automatically and should not be included.
- `library.codon`: Standard library code that should be included with the plugin.
  It is recommended to put this code in directory `stdlib/<plugin_name>`, whereupon
  the value of this parameter would be `"stdlib"`.
- `library.link`: Libraries to be linked when compiling to an executable. The string
  `{root}` will be replaced with the path to the TOML configuration file. For example,
  a value similar to `{root}/build/libmyplugin.a` might be used, assuming the plugin
  also builds a static library containing necessary runtime functions. A string or
  array of strings overrides the default runtime library argument. Explicit link
  arguments remain usable even without a runtime library. For compatibility,
  `library.link = false` means `library.runtime = ""` when `library.runtime` is omitted.
  Search paths come from the runtime library, not the compiler library.

For separate compiler and runtime libraries:

```toml
[library]
compiler = "build/libmyplugin_compiler"
runtime = "build/libmyplugin_runtime"
```

For a compiler-only plugin, set `runtime = ""`. A runtime-only plugin can omit both
`compiler` and `cpp`. In C++, `DSL::Info::compilerDylibPath` and `runtimeDylibPath`
are optional: unset values inherit `dylibPath`, while empty strings disable the
corresponding library. This preserves legacy configuration and normal source
initializers, not the binary layout of `DSL::Info`; plugins using that metadata
must be rebuilt against the matching headers.

Here is an example configuration file for the validate pass shown in the
[Codon IR docs](ir.md#bidirectionality):

``` toml
[about]
name = "MyValidate"
description = "my validation plugin"
version = "0.0.1"
url = "https://example.com"
supported = ">=0.18.0"

[library]
cpp = "build/libmyvalidate"
```

## Implementation

Plugins must extend the `codon::DSL` class to implement their functionality, and provide
a function `extern "C" std::unique_ptr<codon::DSL> load()` that returns an instance of
their subclass. The `load()` function is invoked by Codon automatically when it loads a
plugin from a shared library.

Continuing on the same validate example from above, let's implement the pass as a
plugin. We can use the same pass code in the section linked above, and create a subclass
of the `codon::DSL` class while adding a function `load()` to our library that
returns an instance of it:

``` cpp
class MyValidate : public codon::DSL {
public:
  void addIRPasses(transform::PassManager *pm, bool debug) override {
    std::string insertBefore = debug ? "" : "core-folding-pass-group";
    pm->registerPass(std::make_unique<ValidateFoo>(), insertBefore);
  }
};

extern "C" std::unique_ptr<codon::DSL> load() {
  return std::make_unique<MyValidate>();
}
```

The `codon::DSL` class has methods for adding Codon IR passes, LLVM IR passes and even new
syntax features like keywords (hence the name DSL, since this class effectively
enables the creation of domain-specific languages within Codon). In this case,
we insert our pass into the Codon IR pass manager before the standard folding pass.

A `CMakeLists.txt` is also required, which specifies how to build the plugin to a shared
library with CMake. A complete implementation of this example plugin, along with the `CMakeLists.txt`,
can be found [on GitHub](https://github.com/exaloop/example-codon-plugin).

### Adding LLVM passes

Plugins can add new LLVM passes by overriding the `void addLLVMPasses(llvm::PassBuilder *pb, bool debug)`
method of the `codon::DSL` class. Refer to the
[`llvm::PassBuilder` docs](https://llvm.org/doxygen/classllvm_1_1PassBuilder.html) for details on adding
passes. This hook runs before analysis registration and pipeline construction, so
plugins can also use `registerAnalysisRegistrationCallback` to register analyses. The hook is called for
each LLVM pipeline construction. Callbacks should not modify global LLVM command-line options.
