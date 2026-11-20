---
title: Exceptions and resources
---

Codon distinguishes errors discovered while compiling from exceptions raised
by a running program. An unsupported operation between known types is a
compilation error; invalid input, missing files and similar conditions can
instead be handled with Python's exception syntax.

This distinction is useful when diagnosing a failure: a `try` block can handle
an exception from native code, but cannot make a program with a type-checking
error compile.

## Validating input

An exception can communicate why a value cannot be used, leaving the caller
to decide how to report or recover from the problem:

``` python
def parse_count(text: str) -> int:
    count = int(text)
    if count < 0:
        raise ValueError("count must be nonnegative")
    return count

try:
    count = parse_count("-3")
except ValueError as error:
    print(f"Invalid count: {error}")
else:
    print(f"Count: {count}")

# Invalid count: count must be nonnegative
```

The handler covers both a failed integer conversion and the explicit
nonnegative check. The `else` block runs when the protected operation succeeds,
which keeps successful processing separate from error reporting. Catching the
exception type expected at this boundary also allows unrelated failures to
reach their own callers instead of being hidden.

Within a handler, a bare `raise` propagates the current exception again. This
can be useful when a layer adds logging but leaves recovery to a higher-level
caller. If an absent result is an ordinary outcome rather than an error,
[Optional](generics.md#values-that-may-be-absent) is another way to express it.

## Working with files

A `with` statement keeps a resource's lifetime together with the code that
uses it. For example, suppose `counts.txt` contains:

``` text title="counts.txt"
2
3
5
```

The following function reads and sums its nonblank lines:

``` python
def sum_file(filename: str) -> int:
    total = 0
    with open(filename, encoding="utf-8") as stream:
        for line in stream:
            text = line.strip()
            if text:
                total += int(text)
    return total

print(sum_file("counts.txt"))  # 10
```

The stream closes when control leaves the `with` block, including when a
conversion raises an exception or the function returns early. Reading the
file line by line also avoids constructing a list containing the entire file.
If decoding fails or a line is not an integer, the exception remains available
to the caller; closing the file does not turn that failure into a successful
result.

Specifying an encoding makes the text format explicit. Binary files use a
mode such as `"rb"` and return `bytes` rather than `str`, without performing a
text-decoding step. Native [pathlib](../libraries/stdlib.md) also provides path
objects and filesystem operations when an application needs more than a file
name.

For cleanup that is not represented by a context manager, a `finally` block
runs as control leaves its associated `try` statement, whether processing
succeeds, raises or returns. Keeping cleanup in `with` or `finally` avoids
having to repeat it across each exit path.

## Diagnosing failures

Debug mode retains source information and disables optimizations, making it a
useful starting point for investigating a failure:

``` bash
codon run -debug program.py
```

Normal exception handling remains available in release builds as well. The
separate `-disable-exceptions` option removes that mechanism: raising an
exception traps rather than transferring control to a handler. Programs that
depend on input validation or recovery should retain exception handling; the
[compiler options](../start/options.md) describe the relevant settings.
