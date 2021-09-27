Calling Seq from C/C++
======================

Calling C/C++ from Seq is quite easy with ``from C import``, but Seq can also be called from C/C++ code. To make a Seq function externally visible, simply annotate it with ``@export``:

.. code-block:: seq

    @export
    def foo(n: int):
        for i in range(n):
            print(i * i)
        return n * n

Note that only top-level, non-generic functions can be exported. Now we can create a shared library containing ``foo`` (assuming source file *foo.seq*):

.. code-block:: bash

    seqc build -o foo.o foo.seq
    gcc -shared -lseqrt -lomp foo.o -o libfoo.so

(The last command might require an additional ``-L/path/to/seqrt/lib/`` argument if ``libseqrt`` is not installed on a standard path.)

Now we can call ``foo`` from a C program:

.. code-block:: C

    #include <stdint.h>
    #include <stdio.h>

    int64_t foo(int64_t);

    int main() {
      printf("%llu\n", foo(10));
    }

Compile:

.. code-block:: bash

    gcc -o foo -L. -lfoo foo.c

Now running ``./foo`` should invoke ``foo()`` as defined in Seq, with an argument of ``10``.

Converting types
----------------

The following table shows the conversions between Seq and C/C++ types:

============  ============
   Seq        C/C++
------------  ------------
``int``       ``int64_t``
``float``     ``double``
``bool``      ``bool``
``byte``      ``int8_t``
``str``       ``{int64_t, char*}``
``seq``       ``{int64_t, char*}``
``class``     Pointer to corresponding tuple
``@tuple``    Struct of fields
============  ============
