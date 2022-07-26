Codon extends the core Python language with a pipe operator. You can
chain multiple functions and generators to form a pipeline. Pipeline
stages can be regular functions or generators. In the case of standard
functions, the function is simply applied to the input data and the
result is carried to the remainder of the pipeline, akin to F#\'s
functional piping. If, on the other hand, a stage is a generator, the
values yielded by the generator are passed lazily to the remainder of
the pipeline, which in many ways mirrors how piping is implemented in
Bash. Note that Codon ensures that generator pipelines do not collect
any data unless explicitly requested, thus allowing the processing of
terabytes of data in a streaming fashion with no memory and minimal CPU
overhead.

``` python
def add1(x):
    return x + 1

2 |> add1  # 3; equivalent to add1(2)

def calc(x, y):
    return x + y**2
2 |> calc(3)       # 11; equivalent to calc(2, 3)
2 |> calc(..., 3)  # 11; equivalent to calc(2, 3)
2 |> calc(3, ...)  # 7; equivalent to calc(3, 2)

def gen(i):
    for i in range(i):
        yield i

5 |> gen |> print # prints 0 1 2 3 4 separated by newline
range(1, 4) |> iter |> gen |> print(end=' ')  # prints 0 0 1 0 1 2 without newline
[1, 2, 3] |> print   # prints [1, 2, 3]
range(100000000) |> print  # prints range(0, 100000000)
range(100000000) |> iter |> print  # not only prints all those numbers, but it uses almost no memory at all
```

Codon will chain anything that implements `__iter__`, and the compiler
will optimize out generators whenever possible. Combinations of pipes
and generators can be used to implement efficient streaming pipelines.

{% hint style="warning" %}
The Codon compiler may perform optimizations that change the order of
elements passed through a pipeline. Therefore, it is best to not rely on
order when using pipelines. If order needs to be maintained, consider
using a regular loop or passing an index alongside each element sent
through the pipeline.
{% endhint %}

# Parallel pipelines

CPython and many other implementations alike cannot take advantage of
parallelism due to the infamous global interpreter lock, a mutex that
prevents multiple threads from executing Python bytecode at once. Unlike
CPython, Codon has no such restriction and supports full multithreading.
To this end, Codon supports a *parallel* pipe operator `||>`, which is
semantically similar to the standard pipe operator except that it allows
the elements sent through it to be processed in parallel by the
remainder of the pipeline. Hence, turning a serial program into a
parallel one often requires the addition of just a single character in
Codon. Further, a single pipeline can contain multiple parallel pipes,
resulting in nested parallelism.

``` python
range(100000) |> iter ||> print              # prints all these numbers, probably in random order
range(100000) |> iter ||> process ||> clean  # runs process in parallel, and then cleans data in parallel
```

Codon will automatically schedule the `process` and `clean` functions to
execute as soon as possible. You can control the number of threads via
the `OMP_NUM_THREADS` environment variable.

Internally, the Codon compiler uses an OpenMP task backend to generate
code for parallel pipelines. Logically, parallel pipe operators are
similar to parallel-for loops: the portion of the pipeline after the
parallel pipe is outlined into a new function that is called by the
OpenMP runtime task spawning routines (as in `#pragma omp task` in C++),
and a synchronization point (`#pragma omp taskwait`) is added after the
outlined segment.
