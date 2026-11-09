# pyobj

Standard library class

class pyobj[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj "Copy/link to this API element")

```python
class pyobj
```

Fields 1

`p` `Ptr[u8]`

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__new__ "Copy/link to this API element")

```python
__new__()
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__init__ "Copy/link to this API element")

```python
__init__(self: Self, p: Ptr[u8], steal: bool = False, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

Methods 5

exc\_wrap[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-exc_wrap "Copy/link to this API element")

```python
exc_wrap(retval: T, T: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

to\_str[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-to_str "Copy/link to this API element")

```python
to_str(self: Self, errors: str, empty: str = "", Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

```python
to_str(p: cobj, errors: str, empty: str = "")
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

exc\_check[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-exc_check "Copy/link to this API element")

```python
exc_check()
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

incref[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-incref "Copy/link to this API element")

```python
incref(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

```python
incref(ptr: Ptr[u8])
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

decref[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-decref "Copy/link to this API element")

```python
decref(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

```python
decref(ptr: Ptr[u8])
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

Special methods 68

\_\_raw\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__raw__ "Copy/link to this API element")

```python
__raw__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_del\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__del__ "Copy/link to this API element")

```python
__del__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__radd__ "Copy/link to this API element")

```python
__radd__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rsub__ "Copy/link to this API element")

```python
__rsub__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_matmul\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__matmul__ "Copy/link to this API element")

```python
__matmul__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rmatmul\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rmatmul__ "Copy/link to this API element")

```python
__rmatmul__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rfloordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rfloordiv__ "Copy/link to this API element")

```python
__rfloordiv__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rtruediv\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rtruediv__ "Copy/link to this API element")

```python
__rtruediv__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rmod\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rmod__ "Copy/link to this API element")

```python
__rmod__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rdivmod\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rdivmod__ "Copy/link to this API element")

```python
__rdivmod__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rpow\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rpow__ "Copy/link to this API element")

```python
__rpow__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_invert\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__invert__ "Copy/link to this API element")

```python
__invert__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_lshift\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__lshift__ "Copy/link to this API element")

```python
__lshift__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rlshift\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rlshift__ "Copy/link to this API element")

```python
__rlshift__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rshift\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rshift__ "Copy/link to this API element")

```python
__rshift__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rrshift\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rrshift__ "Copy/link to this API element")

```python
__rrshift__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__and__ "Copy/link to this API element")

```python
__and__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rand\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rand__ "Copy/link to this API element")

```python
__rand__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__xor__ "Copy/link to this API element")

```python
__xor__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_rxor\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__rxor__ "Copy/link to this API element")

```python
__rxor__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_ror\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__ror__ "Copy/link to this API element")

```python
__ror__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_iadd\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__iadd__ "Copy/link to this API element")

```python
__iadd__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_isub\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__isub__ "Copy/link to this API element")

```python
__isub__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_imul\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__imul__ "Copy/link to this API element")

```python
__imul__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_imatmul\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__imatmul__ "Copy/link to this API element")

```python
__imatmul__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_ifloordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__ifloordiv__ "Copy/link to this API element")

```python
__ifloordiv__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_itruediv\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__itruediv__ "Copy/link to this API element")

```python
__itruediv__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_imod\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__imod__ "Copy/link to this API element")

```python
__imod__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_ipow\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__ipow__ "Copy/link to this API element")

```python
__ipow__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_ilshift\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__ilshift__ "Copy/link to this API element")

```python
__ilshift__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_irshift\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__irshift__ "Copy/link to this API element")

```python
__irshift__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_iand\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__iand__ "Copy/link to this API element")

```python
__iand__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_ixor\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__ixor__ "Copy/link to this API element")

```python
__ixor__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__int__ "Copy/link to this API element")

```python
__int__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__float__ "Copy/link to this API element")

```python
__float__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_index\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__index__ "Copy/link to this API element")

```python
__index__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_length\_hint\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__length_hint__ "Copy/link to this API element")

```python
__length_hint__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key, v, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__from_py__ "Copy/link to this API element")

```python
__from_py__(p: cobj)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__call__ "Copy/link to this API element")

```python
__call__(self: Self, *args, **kwargs, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/pyobj/#class-pyobj-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

Source: https://docs.exaloop.io/libraries/api/classes/pyobj/
