---
title: Jupyter
---

Codon's Jupyter component exposes a native kernel through the
`codon jupyter` subcommand. A Jupyter frontend and a Codon installation that
includes this component are needed; source builds can enable it by following
the [Jupyter build instructions](../developers/build.md#jupyter-support).

## Register the kernel

Create a directory called `codon-kernel` containing the following `kernel.json`.
The first entry in `argv` should be the absolute path to the installed Codon
executable, since a Jupyter server may have a different `PATH` from an
interactive shell:

``` json
{
    "display_name": "Codon",
    "argv": [
        "/path/to/codon",
        "jupyter",
        "{connection_file}"
    ],
    "language": "python"
}
```

Jupyter can install this kernel specification into the current user's data
directory and list the resulting registration:

``` bash
python3 -m jupyter kernelspec install --user --name codon ./codon-kernel
python3 -m jupyter kernelspec list
```

The `{connection_file}` placeholder is filled in by Jupyter when it launches
the kernel; it should remain unchanged in the specification. After
registration, **Codon** appears in the frontend's kernel selector.

## Using plugins

Plugins can also optionally be specified, as in:

``` json
{
    "display_name": "Codon",
    "argv": [
        "/path/to/codon",
        "jupyter",
        "-plugin", "/path/to/plugin",
        "{connection_file}"
    ],
    "language": "python"
}
```

The plugin path should also be absolute. If Python interoperability is needed
inside a notebook, its [runtime environment](python/python-from-codon.md#configure-the-python-runtime)
should be configured in the process that launches Jupyter so the kernel
inherits the same settings.

## Session state and troubleshooting

Cells execute in a persistent JIT session, so definitions and values from an
earlier cell can be used by later cells. Restarting the kernel clears that
state. As with an ordinary Codon program, a new input type can require a new
specialization; the [language guides](../language/overview.md) describe the
native type system and its differences from Python.

If a kernel fails to start, checking `codon jupyter --help` in the intended
installation and the executable path in `kernel.json` helps distinguish a
missing component from a registration problem. Jupyter's server log contains
the kernel process's startup diagnostics. An obsolete registration can be
removed with:

``` bash
python3 -m jupyter kernelspec uninstall codon
```
