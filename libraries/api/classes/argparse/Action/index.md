# Action

Standard library class `argparse.Action` [View source](https://github.com/exaloop/codon/blob/master/stdlib/argparse.codon)

class Action[](https://docs.exaloop.io/libraries/api/classes/argparse/Action/#class-action "Copy/link to this API element")

```python
class Action
```

Fields 15

`option_strings` `List[str]`

`dest` `str`

`nargs` `str`

`const` `ArgumentValue`

`default` `ArgumentValue`

`type` `Callable[str, ArgumentValue]`

`type_name` `str`

`choices` `_Choices`

`has_choices` `bool`

`required` `bool`

`help` `Optional[str]`

`metavar` `List[str]`

`deprecated` `bool`

`kind` `int`

`version` `Optional[str]`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/argparse/Action/#class-action-__init__ "Copy/link to this API element")

```python
__init__(self: Self, option_strings, dest: str, nargs = None, const = None, default = None, choices = None, required: bool = False, help: Optional[str] = None, metavar = None, deprecated: bool = False, **kwargs, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

Methods 1

format\_usage[](https://docs.exaloop.io/libraries/api/classes/argparse/Action/#class-action-format_usage "Copy/link to this API element")

```python
format_usage(self: Self, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

Special methods 3

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/argparse/Action/#class-action-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: Action, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/argparse/Action/#class-action-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: Action, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/classes/argparse/Action/#class-action-__call__ "Copy/link to this API element")

```python
__call__(self: Self, parser, namespace: Namespace, values: ArgumentValue, option_string: Optional[str] = None, Self: type)
```

Added by [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

Source: https://docs.exaloop.io/libraries/api/classes/argparse/Action/
