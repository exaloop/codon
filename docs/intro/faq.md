> **What is the goal of Codon?**

One of the main focuses of Codon is to bridge the gap between usability
and performance. Codon aims to make writing high-performance software
substantially easier, and to provide a common, unified framework for the
development of such software across a range of domains.

> **I want to use Codon, but I have a large Python codebase I don't want to port.**

You can use Codon on a per-function basis via the `@codon` annotation, which
can be used within Python codebases. This will compile only the annotated functions
and automatically handle data conversions to and from Codon. It also allows for
the use of any Codon-specific modules or extensions.

> **How does Codon compare to other Python implementations?**

Unlike many other performance-oriented Python implementations, such as
PyPy or Numba, Codon is a standalone system implemented entirely
independently of regular Python or any dynamic runtime, and therefore has
far greater flexibility to generate optimized code. In fact, Codon will
frequently generate the same code as that from an equivalent C or C++ program.
This design choice also allows Codon to circumvent issues like Python's global
interpretter lock, and thereby to take full advantage of parallelism and multithreading.

> **What about interoperability with other languages and frameworks?**

Interoperability is and will continue to be a priority for the Codon
project. We don't want using Codon to render you unable to use all the
other great frameworks and libraries that exist. Codon supports full
interoperability with Python and C/C++.
