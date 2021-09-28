**Codon**
=========

With a Python-compatible syntax and a host of domain-specific features and optimizations, Codon makes writing high-performance software as easy as writing Python code, and achieves performance comparable to (and in many cases better than) C/C++.

Questions, comments or suggestions? Visit our `Gitter chatroom <https://gitter.im/seq-lang/Seq?utm_source=share-link&utm_medium=link&utm_campaign=share-link>`_.

.. toctree::
   :maxdepth: 1

   intro
   tutorial/index
   parallel
   python
   embed
   cookbook
   build
   stdlib/index

News
----

What's new in 0.11?
^^^^^^^^^^^^^^^^^^^

Version 0.11 again includes a number of upgrades to the parser, type system and compiler backend:

- Parallelism and multithreading support with OpenMP (e.g. ``@par for i in range(10): ...``)
- New PEG parser
- Improved compile-time static evaluation
- Upgrade to LLVM 12 (from LLVM 6)

What's new in 0.10?
^^^^^^^^^^^^^^^^^^^

Version 0.10 brings a slew of improvements to the language and compiler, including:

- Nearly all of Python's syntax is now supported, including empty collections (``[]``, ``{}``), lambda functions (``lambda``), ``*args``/``**kwargs``, ``None`` and much more
- Compiler error messages now pinpoint exactly where an error occurred with compile-time backtraces
- Runtime exceptions now include backtraces with file names and line numbers in debug mode
- GDB and LLDB support
- Various syntax updates to further close the gap with Python
- Numerous standard library improvements

.. caution::
    The default compilation and execution mode is now "debug", which disables most optimizations. Pass the ``-release`` argument to ``codon`` to enable optimizations.

Frequently Asked Questions
--------------------------

    *Can I use Codon for general-purpose computing?*

Yes! While the Codon project was inspired by computational challenges in bioinformatics, it has grown to encompass much of Python's syntax, semantics and modules, making it a useful tool beyond just bioinformatics, particularly when large datasets need to be processed with Python.

    *What is the goal of Codon?*

One of the main focuses of Codon is to bridge the gap between usability and performance. Codon aims to make writing high-performance software substantially easier, and to provide a common, unified framework for the development of such software.

    *Why do we need a whole new language? Why not a library?*

A new language and compiler allow us to provide the programmer with higher-level constructs that are paired with optimizations, e.g. `@par` with :ref:`parallelism`. This type of pairing is difficult to replicate in a library alone, as it often involves large-scale program transformations/optimizations. We can also explore different backends like GPU, TPU or FPGA in a systematic way, in conjunction with these various constructs/optimizations, which is ongoing work.

Our goal is 

    *What about interoperability with other languages and frameworks?*

Interoperability is and will continue to be a priority for the Codon project. We don't want using Codon to render you unable to use all the other great frameworks and libraries that exist. Codon already supports interoperability with C/C++ and Python (see :ref:`interop`).

   *I want to contribute! How do I get started?*

Great! Check out our `contribution guidelines <https://github.com/exaloop/codon/blob/master/CONTRIBUTING.md>`_ and `open issues <https://github.com/exaloop/codon/issues>`_ to get started. Also don't hesitate to drop by our `Gitter chatroom <https://gitter.im/seq-lang/Seq?utm_source=share-link&utm_medium=link&utm_campaign=share-link>`_ if you have any questions.

   *What is planned for the future?*

See the `roadmap <https://github.com/seq-lang/seq/wiki/Roadmap>`_ for information about this.
