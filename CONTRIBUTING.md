# Contributing to Seq

Thank you for considering contributing to Seq! This document contains some helpful information for getting started. The best place to ask questions or get feedback is [our Gitter chatroom](https://gitter.im/seq-lang/Seq). For a high-level outline of the features we aim to add in the future, check the [roadmap](https://github.com/seq-lang/seq/wiki/Roadmap).

## Development workflow

All development is done on the [`develop`](https://github.com/seq-lang/seq/tree/develop) branch. Just before release, we bump the version number, merge into [`master`](https://github.com/seq-lang/seq/tree/master) and tag the build with a tag of the form `vX.Y.Z` where `X`, `Y` and `Z` are the [SemVer](https://semver.org) major, minor and patch numbers, respectively. Our Travis CI build script automatically builds and deploys tagged commits as a new GitHub release via our trusty [@SeqBot](https://github.com/seqbot). It also builds and deploys the documentation to our website.

## Coding standards

All C++ code should be formatted with [ClangFormat](https://clang.llvm.org/docs/ClangFormat.html) using the `.clang-format` file in the root of the repository.

## Writing tests

Tests are written as Seq programs. The [`test/core/`](https://github.com/seq-lang/seq/tree/master/test/core) directory contains some examples. If you add a new test file, be sure to add it to [`test/main.cpp`](https://github.com/seq-lang/seq/blob/master/test/main.cpp) so that it will be executed as part of the test suite. There are two ways to write tests for Seq:

#### New style

Example:

```python
@test
def my_test():
    assert 2 + 2 == 4
my_test()
```

**Semantics:** `assert` statements in functions marked `@test` are not compiled to standard assertions: they don't terminate the program when the condition fails, but instead print source information, fail the test, and move on.

#### Old style

Example:

```python
print(2 + 2)  # EXPECT: 4
```

**Semantics:** The source file is scanned for `EXPECT`s, executed, then the output is compared to the "expected" output. Note that if you have, for example, an `EXPECT` in a loop, you will need to duplicate it however many times the loop is executed. Using `EXPECT` is helpful mainly in cases where you need to test control flow, **otherwise prefer the new style**.

## Pull requests

Pull requests should generally be based on the `develop` branch. Before submitting a pull request, pleace make sure...

- ... to provide a clear description of the purpose of the pull request.
- ... to include tests for any new or changed code.
- ... that all code is formatted as per the guidelines above.

Please be patient with pull request reviews, as our throughput is limited!

## Issues

We use [GitHub's issue tracker](https://github.com/seq-lang/seq/issues), so that's where you'll find the most recent list of open bugs, feature requests and general issues. If applicable, we try to tag each issue with at least one of the following tags:

- `Build`: Issues related to building Seq
- `Codegen`: Issues related to code generation (i.e. after parsing and type checking)
- `Parser`: Issues related to lexing/parsing
- `Library`: Issues related to the Seq standard library
- `Interop`: Issues related to interoperability with other languages or systems
- `Docs`: Issues related to documentation
- `Feature`: New language feature proposals
