Getting Started
===============

Install
-------

Pre-built binaries
^^^^^^^^^^^^^^^^^^

Pre-built binaries for Linux and macOS on x86_64 are available alongside `each release <https://github.com/exaloop/codon/releases>`_. We also have a script for downloading and installing pre-built versions:

.. code-block:: bash

    /bin/bash -c "$(curl -fsSL https://seq-lang.org/install.sh)"

This will install Codon in a new ``.codon`` directory within your home directory.

Building from source
^^^^^^^^^^^^^^^^^^^^

See `Building from Source <build.html>`_.

Usage
-----

The ``codon`` program can either directly ``run`` Codon source in JIT mode:

.. code-block:: bash

    codon run myprogram.codon

The default compilation and run mode is *debug* (``-debug``). Compile and run with optimizations with the ``-release`` option:

.. code-block:: bash

    codon run -release myprogram.codon

``codon`` can also ``build`` executables (ensure you have ``clang`` installed, as it is used for linking):

.. code-block:: bash

    # generate 'myprogram' executable
    codon build -exe myprogram.codon

    # generate 'foo' executable
    codon build -o foo myprogram.codon

``codon`` can produce object files:

.. code-block:: bash

    # generate 'myprogram.o' object file
    codon build -obj myprogram.codon

    # generate 'foo.o' object file
    codon build -o foo.o myprogram.codon

``codon`` can produce LLVM IR:

.. code-block:: bash

    # generate 'myprogram.ll' object file
    codon build -llvm myprogram.codon

    # generate 'foo.ll' object file
    codon build -o foo.ll myprogram.codon

Compile-time definitions
------------------------

``codon`` allows for compile-time definitions via the ``-D`` flag. For example, in the following code:

.. code-block:: seq

    from bio import *
    print(Kmer[SEED_LEN]())

``SEED_LEN`` can be specified on the command line as such: ``codon run -DSEED_LEN=10 myprogram.codon``.
