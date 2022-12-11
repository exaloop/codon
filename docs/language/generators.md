Codon supports generators, and in fact they are heavily optimized in
the compiler so as to typically eliminate any overhead:

``` python
def gen(n):
    i = 0
    while i < n:
        yield i ** 2
        i += 1

print(list(gen(10)))  # prints [0, 1, 4, ..., 81]
print(list(gen(0)))   # prints []
```

You can also use `yield` to implement coroutines: `yield` suspends the
function, while `(yield)` (i.e. with parenthesis) receives a value, as
in Python.

``` python
def mysum(start):
    m = start
    while True:
        a = (yield)     # receives the input of coroutine.send() call
        if a == -1:
            break       # exits the coroutine
        m += a
    yield m

iadder = mysum(0)       # assign a coroutine
next(iadder)            # activate it
for i in range(10):
    iadder.send(i)      # send a value to coroutine
print(iadder.send(-1))  # prints 45
```

Generator expressions are also supported:

``` python
squares = (i ** 2 for i in range(10))
for i,s in enumerate(squares):
    print(i, 'x', i, '=', s)
```
