## Install Codon

=== ":simple-gnometerminal: &nbsp; Command-line tool"

    Use this command to install the Codon CLI that can be used to compile and
    run programs from the command line:

    ``` bash
    /bin/bash -c "$(curl -fsSL https://exaloop.io/install.sh)"
    ```

    Follow the prompts to add the `codon` command to your path.

=== ":simple-python: &nbsp; Python package"

    Use this command to install the `codon` Python package, which can be used to
    compile functions in an existing Python codebase:

    ``` bash
    pip install codon-jit
    ```

    With this package installed, you can use the `@codon.jit` decorator.
    [Learn more &#x2192;](/integrations/python/codon-from-python)

!!! info

    Codon is supported natively on :fontawesome-brands-linux: Linux and :fontawesome-brands-apple: macOS.
    If you are using :fontawesome-brands-windows: Windows,
    we recommend using Codon through [WSL](https://learn.microsoft.com/en-us/windows/wsl/about).

## Run your first program

With the `codon` command installed, we can compile and run a simple hello-world program:

``` python
print('Hello, World!')
```

If we save this simple program to a file called `hello.py`, we can compile and run with
the `codon run` subcommand:

``` bash
codon run hello.py
```

which prints the `Hello, World!` message as expected.

## Enable optimizations

By default, `codon` runs programs without optimizations enabled. You can enable
optimizations with the `-release` flag. Let's look at a slightly more involved example
to see the effect of this flag:

``` python
from time import time

def fib(n):
    return n if n < 2 else fib(n - 1) + fib(n - 2)

t0 = time()
ans = fib(40)
t1 = time()
print(f'Computed fib(40) = {ans} in {t1 - t0} seconds.')
```

This program computes the 40th Fibonacci number using simple recursion. We can run
the program with optimizations enabled like this:

``` bash
codon run -release fib.py
```

Let's see what happens when we run this program using Python and Codon without optimizations[^1]:

<div style="text-align: center;" markdown="1">
| Command                      | Time taken (sec.) | Speedup    |
| ---------------------------- | ----------------: | ---------: |
| `python3.13 fib.py`          |             14.26 |  1$\times$ |
| `codon run fib.py`           |             0.43  | 33$\times$ |
| `codon run -release fib.py`  |             0.31  | 46$\times$ |
</div>

That's a 46$\times$ speedup from using Codon with the `-release` flag!

[^1]: Times were measured on an M1 MacBook Pro.
