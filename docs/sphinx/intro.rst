Getting Started
===============

Install
-------

Pre-built binaries
^^^^^^^^^^^^^^^^^^

Pre-built binaries for Linux and macOS on x86_64 are available alongside `each release <https://github.com/seq-lang/seq/releases>`_. We also have a script for downloading and installing pre-built versions:

.. code-block:: bash

    /bin/bash -c "$(curl -fsSL https://seq-lang.org/install.sh)"

This will install Seq in a new ``.seq`` directory within your home directory.

Building from source
^^^^^^^^^^^^^^^^^^^^

See `Building from Source <build.html>`_.

Usage
-----

The ``seqc`` program can either directly run Seq source in JIT mode:

.. code-block:: bash

    seqc run myprogram.seq

The default compilation and run mode is *debug* (``-debug``). Compile and run with optimizations with the ``-release`` option:

.. code-block:: bash

    seqc run -release myprogram.seq

``seqc`` can also produce executables (ensure you have ``clang`` installed, as it is used for linking):

.. code-block:: bash

    # generate 'myprogram' executable
    seqc build -exe myprogram.seq

    # generate 'foo' executable
    seqc build -o foo myprogram.seq

``seqc`` can produce object files:

.. code-block:: bash

    # generate 'myprogram.o' object file
    seqc build -obj myprogram.seq

    # generate 'foo.o' object file
    seqc build -o foo.o myprogram.seq

``seqc`` can produce LLVM IR:

.. code-block:: bash

    # generate 'myprogram.ll' object file
    seqc build -llvm myprogram.seq

    # generate 'foo.ll' object file
    seqc build -o foo.ll myprogram.seq

Compile-time definitions
------------------------

``seqc`` allows for compile-time definitions via the ``-D`` flag. For example, in the following code:

.. code-block:: seq

    from bio import *
    print(Kmer[SEED_LEN]())

``SEED_LEN`` can be specified on the command line as such: ``seqc run -DSEED_LEN=10 myprogram.seq``.
