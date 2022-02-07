To build:

```bash
$ pip install cython
$ python interop/setup.py build_ext --inplace --force
```

To use:

```python
from interop.decorator import codon

@codon
def ...
```
