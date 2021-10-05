Setup
=====

Installation
------------

Simple!

.. code:: bash

    /bin/bash -c "$(curl -fsSL https://seq-lang.org/install.sh)"

If you want to use Python interop, you also need to point
``CODON_PYTHON`` to the Python library (typically called
``libpython3.9m.so`` or similar). The Codon repository contains a
`Python script <https://github.com/exaloop/codon/blob/develop/test/python/find-python-library.py>`_
that will identify and print the path to this library.

Usage
-----

Assuming that Codon was properly installed, you can use it as follows:

.. code:: bash

    codon run foo.codon  # Compile and run foo.codon
    codon run -release foo.codon  # Compile and run foo.codon with optimizations
    codon build -exe file.codon  # Compile foo.codon to executable "foo"

Note that the ``-exe`` option requires ``clang`` to be installed, and
the ``LIBRARY_PATH`` environment variable to point to the Codon runtime
library (installed by default at ``~/.codon/lib/codon``).
