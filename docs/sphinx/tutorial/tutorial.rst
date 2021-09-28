Tutorial
========

.. _pipeline:

Pipelines
^^^^^^^^^

Pipeline stages in Codon can be regular functions or generators. In the case of standard functions, the function is simply applied to the input data and the result is carried to the remainder of the pipeline, akin to F#'s functional piping. If, on the other hand, a stage is a generator, the values yielded by the generator are passed lazily to the remainder of the pipeline, which in many ways mirrors how piping is implemented in Bash. Note that Codon ensures that generator pipelines do not collect any data unless explicitly requested, thus allowing the processing of terabytes of data in a streaming fashion with no memory and minimal CPU overhead.

.. caution::
    The Codon compiler may perform optimizations that change the order of elements passed through a pipeline. Therefore, it is best to not rely on order when using pipelines. If order needs to be maintained, consider using a regular loop or passing an index alongside each element sent through the pipeline.

Parallel Pipelines
^^^^^^^^^^^^^^^^^^

CPython and many other implementations alike cannot take advantage of parallelism due to the infamous global interpreter lock, a mutex that protects accesses to Python objects, preventing multiple threads from executing Python bytecode at once. Unlike CPython, Codon has no such restriction and supports full multithreading. To this end, Codon supports a *parallel* pipe operator ``||>``, which is semantically similar to the standard pipe operator except that it allows the elements sent through it to be processed in parallel by the remainder of the pipeline. Hence, turning a serial program into a parallel one often requires the addition of just a single character in Codon. Further, a single pipeline can contain multiple parallel pipes, resulting in nested parallelism.

Internally, the Codon compiler uses an OpenMP task backend to generate code for parallel pipelines. Logically, parallel pipe operators are similar to parallel-for loops: the portion of the pipeline after the parallel pipe is outlined into a new function that is called by the OpenMP runtime task spawning routines (as in ``#pragma omp task`` in C++), and a synchronization point (``#pragma omp taskwait``) is added after the outlined segment. Lastly, the entire program is implicitly placed in an OpenMP parallel region (``#pragma omp parallel``) that is guarded by a "single" directive (``#pragma omp single``) so that the serial portions are still executed by one thread (this is required by OpenMP as tasks must be bound to an enclosing parallel region).

Type extensions
^^^^^^^^^^^^^^^

Codon provides an ``@extend`` annotation that allows programmers to add and modify methods of various types at compile time, including built-in types like ``int`` or ``str``. This actually allows much of the functionality of built-in types to be implemented in Codon as type extensions in the standard library. Here is an example where the ``int`` type is extended to include a ``to`` method that generates integers in a specified range, as well as to override the ``__mul__`` magic method to "intercept" integer multiplications:

.. code-block:: seq

    @extend
    class int:
        def to(self, other: int):
            for i in range(self, other + 1):
                yield i

        def __truediv__(self, other: int):
            print('caught int div!')
            return 42

    for i in (5).to(10):
        print(i)  # 5, 6, ..., 10

    # prints 'caught int div!' then '42'
    print(2 / 3)

Note that all type extensions are performed strictly at compile time and incur no runtime overhead.

Other types
^^^^^^^^^^^

Codon provides arbitrary-width signed and unsigned integers up to ``Int[512]`` and ``UInt[512]``, respectively (note that ``int`` is an ``Int[64]``). Typedefs for common bit widths are provided in the standard library, such as ``i8``, ``i16``, ``u32``, ``u64`` etc.

The ``Ptr[T]`` type in Codon also corresponds to a raw C pointer (e.g. ``Ptr[byte]`` is equivalent to ``char*`` in C). The ``Array[T]`` type represents a fixed-length array (essentially a pointer with a length).

Codon also provides ``__ptr__`` for obtaining a pointer to a variable (as in ``__ptr__(myvar)``) and ``__array__`` for declaring stack-allocated arrays (as in ``__array__[int](10)``).
