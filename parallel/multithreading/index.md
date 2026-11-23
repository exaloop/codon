# Multithreading

Codon supports parallelism and multithreading via OpenMP out of the box. Here's an example:

```python
@par
for i in range(10):
    import threading as thr
    print('hello from thread', thr.get_ident())
```

By default, parallel loops will use all available threads, or use the number of threads specified by the `OMP_NUM_THREADS` environment variable. A specific thread number can be given directly on the `@par` line as well:

```python
@par(num_threads=5)
for i in range(10):
    import threading as thr
    print('hello from thread', thr.get_ident())
```

`@par` supports several OpenMP parameters, including:

-   `num_threads` (int): the number of threads to use when running the loop
-   `schedule` (str): either _static_, _dynamic_, _guided_, _auto_ or _runtime_
-   `chunk_size` (int): chunk size when partitioning loop iterations
-   `ordered` (bool): enables ordered regions within the loop; code wrapped with `@omp.ordered` follows iteration order, while other work can remain parallel
-   `collapse` (int): number of loop nests to collapse into a single iteration space

Other OpenMP parameters like `private`, `shared` or `reduction`, are inferred automatically by the compiler. For example, the following loop

```python
total = 0
@par(num_threads=2)
for value in range(1000):
    total += value

print(total)  # 499500
```

generates a reduction for `total`: workers accumulate partial results that are combined after the loop. Floating-point reductions may combine values in a different order from a serial loop, so their last few bits can differ even when the calculation is otherwise equivalent.

warning

Operations that change a shared container's structure, such as appending to a list or inserting into a dictionary, need synchronization. Updates to the same element from several iterations can also race. A lock or critical section can protect such operations; the compiler does not automatically make arbitrary shared mutations thread-safe.

When each iteration produces an independent result, preallocating the output often avoids the need for a shared append operation:

```python
values = [3, 5, 7, 9]
squared = [0] * len(values)

@par(num_threads=2)
for index in range(len(values)):
    squared[index] = values[index] * values[index]

print(squared)  # [9, 25, 49, 81]
```

Each iteration writes a different slot, and neither list changes size during the loop. The result therefore retains input order even though the iterations may execute in a different order.

Here is an example that finds the number of primes below a user-defined limit, using a parallel loop on four threads with a dynamic schedule and chunk size of 100:

```python
from sys import argv

def is_prime(number: int) -> bool:
    if number < 2:
        return False
    divisor = 2
    while divisor <= number // divisor:
        if number % divisor == 0:
            return False
        divisor += 1
    return True

limit = int(argv[1]) if len(argv) > 1 else 100_000
total = 0

@par(schedule='dynamic', chunk_size=100, num_threads=4)
for i in range(2, limit):
    if is_prime(i):
        total += 1

print(total)
```

With the default limit, this prints `9592`. A static schedule is often a good starting point when iterations take similar amounts of time, while dynamic or guided schedules can help distribute uneven work. Here, composite numbers may return early while primes require more trial divisions. The best chunk size balances that variation against scheduling overhead.

`@par` also supports C/C++ OpenMP pragma strings. For example, the `@par` line in the above example can also be written as:

```python
# same as: @par(schedule='dynamic', chunk_size=100, num_threads=4)
@par('schedule(dynamic, 100) num_threads(4)')
```

## Loop types

`for`\-loops can iterate over arbitrary generators, but OpenMP's parallel loop construct only applies to _imperative_ for-loops of the form `for i in range(a, b, c)` (where `c` is constant). For general parallel for-loops of the form `for i in some_generator()`, a task-based approach is used instead, where each loop iteration is executed as an independent task.

The Codon compiler also converts iterations over lists (`for a in some_list`) to imperative for-loops, meaning these loops can be executed using OpenMP's loop parallelism.

## Custom reductions

Codon can automatically generate efficient reductions for `int` and `float` values. For other data types, user-defined reductions can be specified. A class that supports reductions must include:

-   A default constructor that represents the _zero value_
-   An `__add__` method (assuming `+` is used as the reduction operator)

Here is an example for reducing a new `Vector` type:

```python
@tuple
class Vector:
    x: int
    y: int

    def __new__():
        return Vector(0, 0)

    def __add__(self, other: Vector):
        return Vector(self.x + other.x, self.y + other.y)

v = Vector()
@par
for i in range(100):
    v += Vector(i,i)
print(v)  # (x: 4950, y: 4950)
```

## OpenMP constructs

All of OpenMP's API functions are accessible directly in Codon. For example:

```python
import openmp as omp
print(omp.get_num_threads())
omp.set_num_threads(32)
```

OpenMP's _critical_, _master_, _single_ and _ordered_ constructs can be applied via the corresponding decorators:

```python
import openmp as omp

@omp.critical
def only_run_by_one_thread_at_a_time():
    print('critical!', omp.get_thread_num())

@omp.master
def only_run_by_master_thread():
    print('master!', omp.get_thread_num())

@omp.single
def only_run_by_single_thread():
    print('single!', omp.get_thread_num())

@omp.ordered
def run_ordered_by_iteration(i):
    print('ordered!', i)

@par(ordered=True)
for i in range(100):
    only_run_by_one_thread_at_a_time()
    only_run_by_master_thread()
    only_run_by_single_thread()
    run_ordered_by_iteration(i)
```

For finer-grained locking, consider using the locks from the `threading` module:

```python
from threading import Lock
lock = Lock()  # or RLock for reentrant lock

@par
for i in range(100):
    with lock:
        print('only one thread at a time allowed here')
```

## Thread-local variables

Variables can be marked as _thread-local_ via `threading.ThreadLocal`:

```python
import threading as thr
x: thr.ThreadLocal[int] = 0
```

A `ThreadLocal[T]` variable is treated as if it had type `T`, but reads or writes of the variable are local to each thread. For example:

```python
import threading as thr
import openmp as omp

local_value: thr.ThreadLocal[int] = 0
lock = thr.Lock()

@par(num_threads=4, schedule='static')
for value in range(4):
    local_value = value
    with lock:
        print(f'Worker {omp.get_thread_num()}: value = {local_value}')
```

Each line reports the current worker and the value it stored, but the lines can appear in any order. Reading and writing within the same parallel region avoids relying on how later regions assign iterations to workers. An OpenMP worker number is also distinct from an operating-system thread identifier.

Source: https://docs.exaloop.io/parallel/multithreading/
