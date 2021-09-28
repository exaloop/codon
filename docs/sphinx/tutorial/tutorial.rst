Tutorial
========

Type extensions
---------------

Codon provides an ``@extend`` annotation that allows programmers to add and modify methods of various types at compile time, including built-in types like ``int`` or ``str``. This allows much of the functionality of built-in types to be implemented in Codon as type extensions in the standard library. Here is an example where the ``int`` type is extended to include a ``to`` method that generates integers in a specified range, as well as to override the ``__mul__`` magic method to "intercept" integer multiplications:

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
-----------

Codon provides arbitrary-width signed and unsigned integers up to ``Int[512]`` and ``UInt[512]``, respectively (note that ``int`` is an ``Int[64]``). Typedefs for common bit widths are provided in the standard library, such as ``i8``, ``i16``, ``u32``, ``u64`` etc.

The ``Ptr[T]`` type in Codon also corresponds to a raw C pointer (e.g. ``Ptr[byte]`` is equivalent to ``char*`` in C). The ``Array[T]`` type represents a fixed-length array (essentially a pointer with a length).

Codon also provides ``__ptr__`` for obtaining a pointer to a variable (as in ``__ptr__(myvar)``) and ``__array__`` for declaring stack-allocated arrays (as in ``__array__[int](10)``).
