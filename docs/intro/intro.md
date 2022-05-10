# Getting started

## Install

### Pre-built binaries

Pre-built binaries for Linux and macOS on x86\_64 are available alongside [each release](https://github.com/exaloop/codon/releases). We also have a script for downloading and installing pre-built versions:

```bash
/bin/bash -c "$(curl -fsSL https://codon.dev/install.sh)"
```

This will install Codon in a new `.codon` directory within your home directory.

### Building from source

See [Building from source](../advanced/build.md).

## Usage

The `codon` program can either directly `run` Codon source in JIT mode:

```bash
codon run myprogram.codon
```

The default compilation and run mode is _debug_ (`-debug`). Compile and run with optimizations with the `-release` option:

```bash
codon run -release myprogram.codon
```

`codon` can also `build` executables (ensure you have `clang` installed, as it is used for linking):

```bash
# generate 'myprogram' executable
codon build -exe myprogram.codon

# generate 'foo' executable
codon build -o foo myprogram.codon
```

`codon` can produce object files:

```bash
# generate 'myprogram.o' object file
codon build -obj myprogram.codon

# generate 'foo.o' object file
codon build -o foo.o myprogram.codon
```

`codon` can produce LLVM IR:

```bash
# generate 'myprogram.ll' object file
codon build -llvm myprogram.codon

# generate 'foo.ll' object file
codon build -o foo.ll myprogram.codon
```

## Compile-time definitions

`codon` allows for compile-time definitions via the `-D` flag. For example, in the following code:

```python
print(Int[BIT_WIDTH]())
```

`BIT_WIDTH` can be specified on the command line as such: `codon run -DBIT_WIDTH=10 myprogram.codon`.