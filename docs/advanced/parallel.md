Codon supports parallelism and multithreading via OpenMP out of the box.
Here\'s an example:

``` python
@par
for i in range(10):
    import threading as thr
    print('hello from thread', thr.get_ident())
```

By default, parallel loops will use all available threads, or use the
number of threads specified by the `OMP_NUM_THREADS` environment
variable. A specific thread number can be given directly on the `@par`
line as well:

``` python
@par(num_threads=5)
for i in range(10):
    import threading as thr
    print('hello from thread', thr.get_ident())
```

`@par` supports several OpenMP parameters, including:

-   `num_threads` (int): the number of threads to use when running the
    loop
-   `schedule` (str): either *static*, *dynamic*, *guided*, *auto* or
    *runtime*
-   `chunk_size` (int): chunk size when partitioning loop iterations
-   `ordered` (bool): whether the loop iterations should be executed in
    the same order
-   `collapse` (int): number of loop nests to collapse into a single
    iteration space

Other OpenMP parameters like `private`, `shared` or `reduction`, are
inferred automatically by the compiler. For example, the following loop

``` python
a = 0
@par
for i in range(N):
    a += foo(i)
```

will automatically generate a reduction for variable `a`.

{% hint style="warning" %}
Modifying shared objects like lists or dictionaries within a parallel
section needs to be done with a lock or critical section. See below
for more details.
{% endhint %}

Here is an example that finds the number of primes up to a
user-defined limit, using a parallel loop on 16 threads with a dynamic
schedule and chunk size of 100:

``` python
from sys import argv

def is_prime(n):
    factors = 0
    for i in range(2, n):
        if n % i == 0:
            factors += 1
    return factors == 0

limit = int(argv[1])
total = 0

@par(schedule='dynamic', chunk_size=100, num_threads=16)
for i in range(2, limit):
    if is_prime(i):
        total += 1

print(total)
```

Static schedules work best when each loop iteration takes roughly the
same amount of time, whereas dynamic schedules are superior when each
iteration varies in duration. Since counting the factors of an integer
takes more time for larger integers, we use a dynamic schedule here.

`@par` also supports C/C++ OpenMP pragma strings. For example, the
`@par` line in the above example can also be written as:

``` python
# same as: @par(schedule='dynamic', chunk_size=100, num_threads=16)
@par('schedule(dynamic, 100) num_threads(16)')
```

# Different kinds of loops

`for`-loops can iterate over arbitrary generators, but OpenMP\'s
parallel loop construct only applies to *imperative* for-loops of the
form `for i in range(a, b, c)` (where `c` is constant). For general
parallel for-loops of the form `for i in some_generator()`, a task-based
approach is used instead, where each loop iteration is executed as an
independent task.

The Codon compiler also converts iterations over lists
(`for a in some_list`) to imperative for-loops, meaning these loops can
be executed using OpenMP\'s loop parallelism.

# Custom reductions

Codon can automatically generate efficient reductions for `int` and
`float` values. For other data types, user-defined reductions can be
specified. A class that supports reductions must include:

-   A default constructor that represents the *zero value*
-   An `__add__` method (assuming `+` is used as the reduction operator)

Here is an example for reducing a new `Vector` type:

``` python
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

# OpenMP constructs

All of OpenMP\'s API functions are accessible directly in Codon. For
example:

``` python
import openmp as omp
print(omp.get_num_threads())
omp.set_num_threads(32)
```

OpenMP\'s *critical*, *master*, *single* and *ordered* constructs can be
applied via the corresponding decorators:

``` python
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

For finer-grained locking, consider using the locks from the `threading`
module:

``` python
from threading import Lock
lock = Lock()  # or RLock for reentrant lock

@par
for i in range(100):
    with lock:
        print('only one thread at a time allowed here')
```
