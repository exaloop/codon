**Codon** - a high-performance Python implementation
====================================================

What is Codon?
--------------

Codon is a high-performance Python implementation that compiles Python code to native machine code without any runtime overhead.
The Codon framework grew out of the `Seq <https://seq-lang.org>`_ project; while Seq focuses on genomics and bioinformatics, Codon
can be applied in a number of different areas and is extensible via a plugin system.

Typical speedups over Python are on the order of 10-100x or more, on a single thread. Codon supports native multithreading which can lead
to speedups many times higher still.

What isn't Codon?
-----------------

While Codon supports nearly all of Python's syntax, it is not a drop-in replacement, and large codebases might require modifications
to be run through the Codon compiler. For example, some of Python's modules are not yet implemented within Codon, and a few of Python's
dynamic features are disallowed. The Codon compiler produces detailed error messages to help identify and resolve any incompatibilities.

Questions, comments or suggestions? Visit our `Gitter chatroom <https://gitter.im/seq-lang/Seq?utm_source=share-link&utm_medium=link&utm_campaign=share-link>`_.

.. toctree::
   :maxdepth: 1

   intro
   tutorial/index
   parallel
   python
   embed
   build
   stdlib/index


Frequently Asked Questions
--------------------------

    *What is the goal of Codon?*

One of the main focuses of Codon is to bridge the gap between usability and performance. Codon aims to make writing high-performance software
substantially easier, and to provide a common, unified framework for the development of such software across a range of domains.

    *How does Codon compare to other Python implementations?*

Unlike other performance-oriented Python implementations, such as PyPy or Numba, Codon is a standalone system implemented entirely independently
of regular Python. Since it does not need to interoperate with CPython's runtime, Codon has far greater flexibility to generate optimized code.
In fact, Codon will frequently generate the same code as that from an equivalent C or C++ program. This design choice also allows Codon to circumvent
issues like Python's global interpretter lock, and thereby to take full advantage of parallelism and multithreading.

    *What about interoperability with other languages and frameworks?*

Interoperability is and will continue to be a priority for the Codon project. We don't want using Codon to render you unable to use all the other great
frameworks and libraries that exist. Codon already supports interoperability with C/C++ and Python (see :ref:`interop`).

   *I want to contribute! How do I get started?*

Great! Check out our `contribution guidelines <https://github.com/exaloop/codon/blob/master/CONTRIBUTING.md>`_ and `open issues <https://github.com/exaloop/codon/issues>`_
to get started. Also don't hesitate to drop by our `Gitter chatroom <https://gitter.im/seq-lang/Seq?utm_source=share-link&utm_medium=link&utm_campaign=share-link>`_
if you have any questions.

   *What is planned for the future?*

See the `roadmap <https://github.com/exaloop/codon/wiki/Roadmap>`_ for information about this.
