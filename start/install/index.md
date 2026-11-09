# Install Codon

### Command-line tool

Use this command to install the Codon CLI that can be used to compile and run programs from the command line:

```bash
/bin/bash -c "$(curl -fsSL https://exaloop.io/install.sh)"
```

Follow the prompts to add the `codon` command to your path.

### Python package

Use this command to install the `codon` Python package, which can be used to compile functions in an existing Python codebase:

```bash
python3 -m pip install codon-jit
```

With this package installed, you can use the `@codon.jit` decorator. [Learn more](https://docs.exaloop.io/integrations/python/codon-from-python/).

info

Codon is supported natively on Linux and macOS. If you are using Windows, we recommend using Codon through [WSL](https://learn.microsoft.com/en-us/windows/wsl/about).

## Run your first program

With the `codon` command installed, we can compile and run a simple hello-world program:

```python
print('Hello, World!')
```

If we save this simple program to a file called `hello.py`, we can compile and run with the `codon run` subcommand:

```bash
codon run hello.py
```

This prints the `Hello, World!` message as expected.

## Enable optimizations

By default, `codon` runs programs without optimizations enabled. You can enable optimizations with the `-release` flag. Let's look at a slightly more involved example to see the effect of this flag:

```python
from time import time

def fib(n):
    return n if n < 2 else fib(n - 1) + fib(n - 2)

t0 = time()
ans = fib(40)
t1 = time()
print(f'Computed fib(40) = {ans} in {t1 - t0} seconds.')
```

This program computes the 40th Fibonacci number using simple recursion. We can run the program with optimizations enabled like this:

```bash
codon run -release fib.py
```

The following measurements illustrate the difference between Python, Codon debug mode and Codon release mode on an M1 MacBook Pro[1](https://docs.exaloop.io/start/install/#user-content-fn-1):

| Command | Time taken (sec.) | Speedup |
| --- | --- | --- |
| `python3.13 fib.py` | 14.26 | 1$\times$ |
| `codon run fib.py` | 0.43 | 33$\times$ |
| `codon run -release fib.py` | 0.31 | 46$\times$ |

These are elapsed times reported by the program around `fib(40)`, not the total time spent compiling and launching it. The example illustrates native execution and the release flag, rather than an efficient Fibonacci algorithm; an iterative or memoized implementation would be more suitable when the goal is simply to calculate the result. Performance on other workloads and machines will differ.

The [usage guide](https://docs.exaloop.io/start/usage/#measuring-performance) explains how to separate compilation from execution when measuring an application. From here, [functions and generators](https://docs.exaloop.io/language/functions/) provide a practical introduction to writing native code, while [Python interoperability](https://docs.exaloop.io/integrations/python/codon-from-python/) shows how to use Codon within an existing Python project.

## Footnotes

1.  Times were measured on an M1 MacBook Pro. [↩](https://docs.exaloop.io/start/install/#user-content-fnref-1)

Source: https://docs.exaloop.io/start/install/
