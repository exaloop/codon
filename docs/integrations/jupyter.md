Codon ships with a kernel that can be used by Jupyter, invoked
with the `codon jupyter ...` subcommand.

To add the Codon kernel, add the following `kernel.json` file to
the directory `/path/to/jupyter/kernels/codon/`:

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
