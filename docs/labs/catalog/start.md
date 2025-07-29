# 🧪 Getting started with Codon

<div style="padding:56.25% 0 0 0;position:relative;"><iframe src="https://player.vimeo.com/video/1104825525?badge=0&amp;autopause=0&amp;player_id=0&amp;app_id=58479" frameborder="0" allow="autoplay; fullscreen; picture-in-picture; clipboard-write; encrypted-media; web-share" referrerpolicy="strict-origin-when-cross-origin" style="position:absolute;top:0;left:0;width:100%;height:100%;" title="Getting Started With Codon"></iframe></div><script src="https://player.vimeo.com/api/player.js"></script>

## Install Codon

Run the following shell command in your terminal to install Codon:

``` bash
/bin/bash -c "$(curl -fsSL https://exaloop.io/install.sh)"
```

## Run your first program

Save the following code to file `fib.py`:

``` python
from time import time

def fib(n):
    return n if n < 2 else fib(n - 1) + fib(n - 2)

t0 = time()
ans = fib(40)
t1 = time()
print(f'Computed fib(40) = {ans} in {t1 - t0} seconds.')
```

Run the program in Codon:

``` bash
codon run -release fib.py
```

Compile to an executable:

``` bash
codon build -release -o fib fib.py
```

Run the executable via `./fib`.

## Use Codon's "just-in-time" (JIT) compiler

Install Codon's JIT with `pip`:

``` bash
pip install codon-jit
```

Save the following code to file `primes.py`:

``` python
import codon
from time import time

def is_prime_python(n):
    if n <= 1:
        return False
    for i in range(2, n):
        if n % i == 0:
            return False
    return True

@codon.jit
def is_prime_codon(n):
    if n <= 1:
        return False
    for i in range(2, n):
        if n % i == 0:
            return False
    return True

t0 = time()
ans = sum(1 for i in range(100000, 200000) if is_prime_python(i))
t1 = time()
print(f'[python] {ans} | took {t1 - t0} seconds')

t0 = time()
ans = sum(1 for i in range(100000, 200000) if is_prime_codon(i))
t1 = time()
print(f'[codon]  {ans} | took {t1 - t0} seconds')
```

Run with Python:

``` python
python3 primes.py
```
