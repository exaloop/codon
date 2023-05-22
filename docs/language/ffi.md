Codon can seamlessly call functions from C and Python:

``` python
from C import pow(float, float) -> float
pow(2.0, 2.0)  # 4.0

# Import and rename function
# cobj is a C pointer (void*, char*, etc.)
# None can be used to represent C's void
from C import puts(cobj) -> None as print_line
print_line("hello".c_str())  # prints "hello"; c_str() converts Codon str to C string
```

`from C import` only works if the symbol is available to the program. If
you are running your programs via `codon`, you can link dynamic
libraries with `-l`: `codon run -l /path/to/library.so ...`.

You can also load shared libraries with `dlopen`:

``` python
LIBRARY = "somelib.so"
from C import LIBRARY.mymethod(int, float) -> cobj
from C import LIBRARY.myothermethod(int, float) -> cobj as my2
foo = mymethod(1, 2.2)
foo2 = my2(4, 3.2)
```

{% hint style="warning" %}
When importing C functions, you must explicitly specify
argument and return types.
{% endhint %}

How about Python? If you have set the `CODON_PYTHON` environment
variable to point to the Python library, you can do:

``` python
from python import mymodule.myfunction(str) -> int as foo
print(foo("bar"))
```

You might want to execute more complex Python code within Codon. To that
end, you can use Codon's `@python` annotation:

``` python
@python
def scipy_eigenvalues(i: List[List[float]]) -> List[float]:
    # Code within this block is executed by the Python interpreter,
    # so it must be valid Python code.
    import scipy.linalg
    import numpy as np
    data = np.array(i)
    eigenvalues, _ = scipy.linalg.eig(data)
    return list(eigenvalues)
print(scipy_eigenvalues([[1.0, 2.0], [3.0, 4.0]]))  # [-0.372281, 5.37228]
```

Codon will automatically bridge any object that implements the
`__to_py__` and `__from_py__` magic methods. All standard Codon types
already implement these methods.
