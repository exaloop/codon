# getopt

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/getopt.codon)

class GetoptError[](https://docs.exaloop.io/libraries/api/modules/getopt/#class-getopterror "Copy/link to this API element")

```python
class GetoptError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/getopt/GetoptError/)

Fields 2

`opt` `Any`

`msg` `Any`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/getopt/#class-getopterror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, msg: str, opt: str = "", Self: type)
```

Properties 1

args[](https://docs.exaloop.io/libraries/api/modules/getopt/#class-getopterror-args "Copy/link to this API element")

@property

```python
args(self: Self, Self: type)
```

Special methods 1

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/getopt/#class-getopterror-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

function getopt[](https://docs.exaloop.io/libraries/api/modules/getopt/#function-getopt "Copy/link to this API element")

```python
getopt(args: List[str], shortopts: str, longopts: List[str] = List[str]())
```

@overload

```python
getopt(args: List[str], shortopts: str, longopts: str)
```

function gnu\_getopt[](https://docs.exaloop.io/libraries/api/modules/getopt/#function-gnu_getopt "Copy/link to this API element")

```python
gnu_getopt(args: List[str], shortopts: str, longopts: List[str] = List[str]())
```

@overload

```python
gnu_getopt(args: List[str], shortopts: str, longopts: str)
```

Source: https://docs.exaloop.io/libraries/api/modules/getopt/
