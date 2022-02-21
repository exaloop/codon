To build:

```bash
$ pip install cython
$ python extra/cython/setup.py build_ext --inplace --force
```

To use:

```python
from extra.cython import codon, CodonError

@codon
def ...
```
