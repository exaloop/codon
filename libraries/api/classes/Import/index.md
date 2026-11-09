# Import

Standard library class

class Import[](https://docs.exaloop.io/libraries/api/classes/Import/#class-import "Copy/link to this API element")

@tuple

```python
class Import[file, name]
```

Fields 3

`loaded` `bool`

`file` `Literal[str]`

`name` `Literal[str]`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/Import/#class-import-__new__ "Copy/link to this API element")

@pure @derives @llvm

```python
__new__(loaded: bool, file: Literal[str], name: Literal[str])
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

```llvm
%0 = insertvalue { {=bool} } undef, {=bool} %loaded, 0
ret { {=bool} } %0
```

Special methods 1

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/Import/#class-import-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.types.import_`](https://docs.exaloop.io/libraries/api/modules/internal/types/import_/)

Source: https://docs.exaloop.io/libraries/api/classes/Import/
