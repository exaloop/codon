Setup
=====

Installation
------------

Simple!

.. code:: bash

    /bin/bash -c "$(curl -fsSL https://seq-lang.org/install.sh)"

If you want to use Python interop, you also need to point
``SEQ_PYTHON`` to the Python library (typically called
``libpython3.9m.so`` or similar). The Seq repository contains a
`Python script <https://github.com/seq-lang/seq/blob/develop/test/python/find-python-library.py>`_
that will identify and print the path to this library.

Usage
-----

Assuming that Seq was properly installed, you can use it as follows:

.. code:: bash

    seqc run foo.seq  # Compile and run foo.seq
    seqc run -release foo.seq  # Compile and run foo.seq with optimizations
    seqc build -exe file.seq  # Compile foo.seq to executable "foo"

Note that the ``-exe`` option requires ``clang`` to be installed, and
the ``LIBRARY_PATH`` environment variable to point to the Seq runtime
library (installed by default at ``~/.seq/lib/seq``).
