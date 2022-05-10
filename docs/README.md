# What is Codon?

Codon is a high-performance Python compiler that compiles Python code to
native machine code without any runtime overhead. The Codon framework
grew out of the [Seq](https://seq-lang.org) project; while Seq focuses
on genomics and bioinformatics, Codon can be applied in a number of
different areas and is extensible via a plugin system.

Typical speedups over Python are on the order of 10-100x or more, on a
single thread. Codon supports native multithreading which can lead to
speedups many times higher still.

# What isn't Codon?

While Codon supports nearly all of Python's syntax, it is not a drop-in
replacement, and large codebases might require modifications to be run
through the Codon compiler. For example, some of Python's modules are
not yet implemented within Codon, and a few of Python's dynamic
features are disallowed. The Codon compiler produces detailed error
messages to help identify and resolve any incompatibilities.

Questions, comments or suggestions? Visit our [Discord
server](https://discord.com/invite/8aKr6HEN?utm_source=Discord%20Widget&utm_medium=Connect).

# Frequently Asked Questions

> **What is the goal of Codon?**

One of the main focuses of Codon is to bridge the gap between usability
and performance. Codon aims to make writing high-performance software
substantially easier, and to provide a common, unified framework for the
development of such software across a range of domains.

> **I want to use Codon, but I have a large Python codebase I don't want to port.**

You can use Codon on a per-function basis via the `@codon` annotation, which
can be used within Python codebases. This will compile only the annotated functions
and automatically handle data conversions to and from Codon.

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
other great frameworks and libraries that exist. Codon already supports
full interoperability with C/C++ and Python.

> **I want to contribute! How do I get started?**

Great! Check out our [contribution
guidelines](https://github.com/exaloop/codon/blob/master/CONTRIBUTING.md)
and [open issues](https://github.com/exaloop/codon/issues) to get
started. Also don't hesitate to drop by our [Discord
server](https://discord.com/invite/8aKr6HEN?utm_source=Discord%20Widget&utm_medium=Connect)
if you have any questions.

> **What is planned for the future?**

See the [roadmap](https://github.com/exaloop/codon/wiki/Roadmap) for
information about this.
