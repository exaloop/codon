# numpy.reductions

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/reductions.codon)

variable newaxis[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#variable-newaxis "Copy/link to this API element")

```python
newaxis = None
```

class SumRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-sumredux "Copy/link to this API element")

```python
class SumRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/)

Fields 2

`total` `T`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-sumredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 8

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-sumredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-sumredux-accept "Copy/link to this API element")

```python
accept(self: Self, item, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-sumredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-sumredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-sumredux-done "Copy/link to this API element")

```python
done(self: Self)
```

gradual\_init[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-sumredux-gradual_init "Copy/link to this API element")

```python
gradual_init(self: Self, **kwargs)
```

gradual\_accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-sumredux-gradual_accept "Copy/link to this API element")

```python
gradual_accept(self: Self, curr, item, index: int, **kwargs)
```

loop[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-sumredux-loop "Copy/link to this API element")

```python
loop[partial: Literal[bool], S: type](self: Self, a: Ptr[S], n: int, stride: int)
```

class NanSumRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nansumredux "Copy/link to this API element")

```python
class NanSumRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/)

Fields 2

`total` `T`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nansumredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 8

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nansumredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nansumredux-accept "Copy/link to this API element")

```python
accept(self: Self, item, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nansumredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nansumredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nansumredux-done "Copy/link to this API element")

```python
done(self: Self)
```

gradual\_init[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nansumredux-gradual_init "Copy/link to this API element")

```python
gradual_init(self: Self, **kwargs)
```

gradual\_accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nansumredux-gradual_accept "Copy/link to this API element")

```python
gradual_accept(self: Self, curr, item, index: int, **kwargs)
```

loop[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nansumredux-loop "Copy/link to this API element")

```python
loop[partial: Literal[bool], S: type](self: Self, a: Ptr[S], n: int, stride: int)
```

class ProdRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-prodredux "Copy/link to this API element")

```python
class ProdRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/ProdRedux/)

Fields 2

`total` `T`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-prodredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 8

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-prodredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-prodredux-accept "Copy/link to this API element")

```python
accept(self: Self, item, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-prodredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-prodredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-prodredux-done "Copy/link to this API element")

```python
done(self: Self)
```

gradual\_init[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-prodredux-gradual_init "Copy/link to this API element")

```python
gradual_init(self: Self, **kwargs)
```

gradual\_accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-prodredux-gradual_accept "Copy/link to this API element")

```python
gradual_accept(self: Self, curr, item, index: int, **kwargs)
```

loop[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-prodredux-loop "Copy/link to this API element")

```python
loop[partial: Literal[bool], S: type](self: Self, a: Ptr[S], n: int, stride: int)
```

class NanProdRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanprodredux "Copy/link to this API element")

```python
class NanProdRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanProdRedux/)

Fields 2

`total` `T`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanprodredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 8

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanprodredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanprodredux-accept "Copy/link to this API element")

```python
accept(self: Self, item, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanprodredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanprodredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanprodredux-done "Copy/link to this API element")

```python
done(self: Self)
```

gradual\_init[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanprodredux-gradual_init "Copy/link to this API element")

```python
gradual_init(self: Self, **kwargs)
```

gradual\_accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanprodredux-gradual_accept "Copy/link to this API element")

```python
gradual_accept(self: Self, curr, item, index: int, **kwargs)
```

loop[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanprodredux-loop "Copy/link to this API element")

```python
loop[partial: Literal[bool], S: type](self: Self, a: Ptr[S], n: int, stride: int)
```

class MeanRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-meanredux "Copy/link to this API element")

```python
class MeanRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/MeanRedux/)

Fields 2

`total` `T`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-meanredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 9

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-meanredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-meanredux-accept "Copy/link to this API element")

```python
accept(self: Self, item: T, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-meanredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-meanredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-meanredux-done "Copy/link to this API element")

```python
done(self: Self)
```

gradual\_init[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-meanredux-gradual_init "Copy/link to this API element")

```python
gradual_init(self: Self, **kwargs)
```

gradual\_accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-meanredux-gradual_accept "Copy/link to this API element")

```python
gradual_accept(self: Self, curr, item, index: int, **kwargs)
```

gradual\_result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-meanredux-gradual_result "Copy/link to this API element")

```python
gradual_result(self: Self, curr, count: int)
```

loop[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-meanredux-loop "Copy/link to this API element")

```python
loop[partial: Literal[bool], S: type](self: Self, a: Ptr[S], n: int, stride: int)
```

class NanMeanRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanmeanredux "Copy/link to this API element")

```python
class NanMeanRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanMeanRedux/)

Fields 3

`total` `T`

`T` `type`

`nan_count` `int`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanmeanredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 6

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanmeanredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanmeanredux-accept "Copy/link to this API element")

```python
accept(self: Self, item: T, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanmeanredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanmeanredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanmeanredux-done "Copy/link to this API element")

```python
done(self: Self)
```

loop[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nanmeanredux-loop "Copy/link to this API element")

```python
loop[partial: Literal[bool], S: type](self: Self, a: Ptr[S], n: int, stride: int)
```

class MinRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-minredux "Copy/link to this API element")

```python
class MinRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/MinRedux/)

Fields 2

`m` `Optional[T]`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-minredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 8

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-minredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-minredux-accept "Copy/link to this API element")

```python
accept(self: Self, item: T, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-minredux-result "Copy/link to this API element")

```python
result(self: Self, count: int) -> T
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-minredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-minredux-done "Copy/link to this API element")

```python
done(self: Self)
```

gradual\_init[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-minredux-gradual_init "Copy/link to this API element")

```python
gradual_init(self: Self, **kwargs)
```

gradual\_accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-minredux-gradual_accept "Copy/link to this API element")

```python
gradual_accept(self: Self, curr, item, index: int, **kwargs)
```

loop[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-minredux-loop "Copy/link to this API element")

```python
loop[partial: Literal[bool], S: type](self: Self, a: Ptr[S], n: int, stride: int)
```

class MaxRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-maxredux "Copy/link to this API element")

```python
class MaxRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/MaxRedux/)

Fields 2

`m` `Optional[T]`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-maxredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 8

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-maxredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-maxredux-accept "Copy/link to this API element")

```python
accept(self: Self, item: T, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-maxredux-result "Copy/link to this API element")

```python
result(self: Self, count: int) -> T
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-maxredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-maxredux-done "Copy/link to this API element")

```python
done(self: Self)
```

gradual\_init[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-maxredux-gradual_init "Copy/link to this API element")

```python
gradual_init(self: Self, **kwargs)
```

gradual\_accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-maxredux-gradual_accept "Copy/link to this API element")

```python
gradual_accept(self: Self, curr, item, index: int, **kwargs)
```

loop[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-maxredux-loop "Copy/link to this API element")

```python
loop[partial: Literal[bool], S: type](self: Self, a: Ptr[S], n: int, stride: int)
```

class PTPRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ptpredux "Copy/link to this API element")

```python
class PTPRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/PTPRedux/)

Fields 3

`hi` `Optional[T]`

`lo` `Optional[T]`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ptpredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 6

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ptpredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ptpredux-accept "Copy/link to this API element")

```python
accept(self: Self, item: T, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ptpredux-result "Copy/link to this API element")

```python
result(self: Self, count: int) -> T
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ptpredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ptpredux-done "Copy/link to this API element")

```python
done(self: Self)
```

loop[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ptpredux-loop "Copy/link to this API element")

```python
loop[partial: Literal[bool], S: type](self: Self, a: Ptr[S], n: int, stride: int)
```

class ArgMinRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argminredux "Copy/link to this API element")

```python
class ArgMinRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/ArgMinRedux/)

Fields 3

`m` `Optional[T]`

`i` `int`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argminredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 5

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argminredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argminredux-accept "Copy/link to this API element")

```python
accept(self: Self, item: T, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argminredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argminredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argminredux-done "Copy/link to this API element")

```python
done(self: Self)
```

class ArgMaxRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argmaxredux "Copy/link to this API element")

```python
class ArgMaxRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/ArgMaxRedux/)

Fields 3

`m` `Optional[T]`

`i` `int`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argmaxredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 5

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argmaxredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argmaxredux-accept "Copy/link to this API element")

```python
accept(self: Self, item: T, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argmaxredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argmaxredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-argmaxredux-done "Copy/link to this API element")

```python
done(self: Self)
```

class AnyRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-anyredux "Copy/link to this API element")

```python
class AnyRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/AnyRedux/)

Fields 2

`a` `bool`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-anyredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 5

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-anyredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-anyredux-accept "Copy/link to this API element")

```python
accept(self: Self, item: T, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-anyredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-anyredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-anyredux-done "Copy/link to this API element")

```python
done(self: Self)
```

class AllRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-allredux "Copy/link to this API element")

```python
class AllRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/AllRedux/)

Fields 2

`a` `bool`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-allredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 5

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-allredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-allredux-accept "Copy/link to this API element")

```python
accept(self: Self, item: T, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-allredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-allredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-allredux-done "Copy/link to this API element")

```python
done(self: Self)
```

class NonZeroRedux[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nonzeroredux "Copy/link to this API element")

```python
class NonZeroRedux[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NonZeroRedux/)

Fields 2

`nonzero` `int`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nonzeroredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Methods 7

create[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nonzeroredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nonzeroredux-accept "Copy/link to this API element")

```python
accept(self: Self, item: T, index: int)
```

result[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nonzeroredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

empty[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nonzeroredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

done[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nonzeroredux-done "Copy/link to this API element")

```python
done(self: Self)
```

gradual\_init[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nonzeroredux-gradual_init "Copy/link to this API element")

```python
gradual_init(self: Self, **kwargs)
```

gradual\_accept[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-nonzeroredux-gradual_accept "Copy/link to this API element")

```python
gradual_accept(self: Self, curr, item, index: int, **kwargs)
```

function sum[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-sum "Copy/link to this API element")

```python
sum[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, initial = 0, where = util._NoValue())
```

function nansum[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-nansum "Copy/link to this API element")

```python
nansum[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, initial = 0, where = util._NoValue())
```

function prod[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-prod "Copy/link to this API element")

```python
prod[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, initial = 1, where = util._NoValue())
```

function nanprod[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-nanprod "Copy/link to this API element")

```python
nanprod[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, initial = 1, where = util._NoValue())
```

function mean[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-mean "Copy/link to this API element")

```python
mean[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, where = util._NoValue())
```

function nanmean[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-nanmean "Copy/link to this API element")

```python
nanmean[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, where = util._NoValue())
```

function var[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-var "Copy/link to this API element")

```python
var[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, ddof: int = 0, where = util._NoValue())
```

function nanvar[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-nanvar "Copy/link to this API element")

```python
nanvar[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, ddof: int = 0, where = util._NoValue())
```

function std[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-std "Copy/link to this API element")

```python
std[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, ddof: int = 0, where = util._NoValue())
```

function nanstd[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-nanstd "Copy/link to this API element")

```python
nanstd[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, ddof: int = 0, where = util._NoValue())
```

function min[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-min "Copy/link to this API element")

```python
min[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, ddof: int = 0, initial = util._NoValue(), where = util._NoValue())
```

function max[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-max "Copy/link to this API element")

```python
max[dtype: type = NoneType, keepdims: Literal[bool] = False](a, axis = None, out = None, ddof: int = 0, initial = util._NoValue(), where = util._NoValue())
```

function ptp[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-ptp "Copy/link to this API element")

```python
ptp[keepdims: Literal[bool] = False](a, axis = None, out = None)
```

function argmin[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-argmin "Copy/link to this API element")

```python
argmin[keepdims: Literal[bool] = False](a, axis = None, out = None)
```

function argmax[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-argmax "Copy/link to this API element")

```python
argmax[keepdims: Literal[bool] = False](a, axis = None, out = None)
```

function any[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-any "Copy/link to this API element")

```python
any[keepdims: Literal[bool] = False](a, axis = None, out = None, where = util._NoValue())
```

function all[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-all "Copy/link to this API element")

```python
all[keepdims: Literal[bool] = False](a, axis = None, out = None, where = util._NoValue())
```

function count\_nonzero[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-count_nonzero "Copy/link to this API element")

```python
count_nonzero[keepdims: Literal[bool] = False](a, axis = None)
```

function median[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-median "Copy/link to this API element")

```python
median[keepdims: Literal[bool] = False](a, axis = None, out = None, overwrite_input: bool = False)
```

function nanmedian[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-nanmedian "Copy/link to this API element")

```python
nanmedian[keepdims: Literal[bool] = False](a, axis = None, out = None, overwrite_input: bool = False)
```

function quantile[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-quantile "Copy/link to this API element")

```python
quantile[keepdims: Literal[bool] = False](a, q, axis = None, out = None, overwrite_input: bool = False, method: str = "linear", interpolation = None)
```

function nanquantile[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-nanquantile "Copy/link to this API element")

```python
nanquantile[keepdims: Literal[bool] = False](a, q, axis = None, out = None, overwrite_input: bool = False, method: str = "linear", interpolation = None)
```

function percentile[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-percentile "Copy/link to this API element")

```python
percentile[keepdims: Literal[bool] = False](a, q, axis = None, out = None, overwrite_input: bool = False, method: str = "linear", interpolation = None)
```

function nanpercentile[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#function-nanpercentile "Copy/link to this API element")

```python
nanpercentile[keepdims: Literal[bool] = False](a, q, axis = None, out = None, overwrite_input: bool = False, method: str = "linear", interpolation = None)
```

class ndarray[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray "Copy/link to this API element")

@extend

```python
class ndarray[dtype, ndim]
```

Extends [`numpy.ndarray.ndarray`](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/)

Methods 15

sum[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-sum "Copy/link to this API element")

```python
sum[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, axis = None, out = None, initial = 0, where = util._NoValue())
```

prod[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-prod "Copy/link to this API element")

```python
prod[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, axis = None, out = None, initial = 1, where = util._NoValue())
```

mean[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-mean "Copy/link to this API element")

```python
mean[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, axis = None, out = None, where = util._NoValue())
```

nanmean[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-nanmean "Copy/link to this API element")

```python
nanmean[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, axis = None, out = None, where = util._NoValue())
```

var[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-var "Copy/link to this API element")

```python
var[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, axis = None, out = None, ddof: int = 0, where = util._NoValue())
```

nanvar[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-nanvar "Copy/link to this API element")

```python
nanvar[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, axis = None, out = None, ddof: int = 0, where = util._NoValue())
```

std[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-std "Copy/link to this API element")

```python
std[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, axis = None, out = None, ddof: int = 0, where = util._NoValue())
```

nanstd[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-nanstd "Copy/link to this API element")

```python
nanstd[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, axis = None, out = None, ddof: int = 0, where = util._NoValue())
```

min[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-min "Copy/link to this API element")

```python
min[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, axis = None, out = None, ddof: int = 0, initial = util._NoValue(), where = util._NoValue())
```

ptp[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-ptp "Copy/link to this API element")

```python
ptp[keepdims: Literal[bool] = False](self: Self, axis = None, out = None)
```

max[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-max "Copy/link to this API element")

```python
max[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, axis = None, out = None, ddof: int = 0, initial = util._NoValue(), where = util._NoValue())
```

argmin[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-argmin "Copy/link to this API element")

```python
argmin[keepdims: Literal[bool] = False](self: Self, axis = None, out = None)
```

argmax[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-argmax "Copy/link to this API element")

```python
argmax[keepdims: Literal[bool] = False](self: Self, axis = None, out = None)
```

any[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-any "Copy/link to this API element")

```python
any[keepdims: Literal[bool] = False](self: Self, axis = None, out = None, where = util._NoValue())
```

all[](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/#class-ndarray-all "Copy/link to this API element")

```python
all[keepdims: Literal[bool] = False](self: Self, axis = None, out = None, where = util._NoValue())
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/reductions/
