# ArgumentParser

Standard library class `argparse.ArgumentParser` [View source](https://github.com/exaloop/codon/blob/master/stdlib/argparse.codon)

class ArgumentParser[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser "Copy/link to this API element")

```python
class ArgumentParser
```

Fields 13

`prog` `str`

`usage` `Optional[str]`

`description` `Optional[str]`

`epilog` `Optional[str]`

`prefix_chars` `str`

`fromfile_prefix_chars` `Optional[str]`

`argument_default` `ArgumentValue`

`conflict_handler` `str`

`allow_abbrev` `bool`

`exit_on_error` `bool`

`suggest_on_error` `bool`

`color` `bool`

`formatter_class` `Callable[str, HelpFormatter]`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-__init__ "Copy/link to this API element")

```python
__init__(self: Self, prog: Optional[str] = None, usage: Optional[str] = None, description: Optional[str] = None, epilog: Optional[str] = None, parents = None, formatter_class = HelpFormatter, prefix_chars: str = "-", fromfile_prefix_chars: Optional[str] = None, argument_default = None, conflict_handler: str = "error", add_help: bool = True, allow_abbrev: bool = True, exit_on_error: bool = True, suggest_on_error: bool = False, color: bool = True, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

Methods 18

add\_argument[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-add_argument "Copy/link to this API element")

```python
add_argument(self: Self, *names, **kwargs, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

add\_argument\_group[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-add_argument_group "Copy/link to this API element")

```python
add_argument_group(self: Self, title: str = "", description: Optional[str] = None, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

add\_mutually\_exclusive\_group[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-add_mutually_exclusive_group "Copy/link to this API element")

```python
add_mutually_exclusive_group(self: Self, required: bool = False, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

add\_subparsers[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-add_subparsers "Copy/link to this API element")

```python
add_subparsers(self: Self, title: Optional[str] = None, description: Optional[str] = None, prog: Optional[str] = None, parser_class = None, dest: str = SUPPRESS, required: bool = False, help: Optional[str] = None, metavar = None, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

set\_defaults[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-set_defaults "Copy/link to this API element")

```python
set_defaults(self: Self, **kwargs, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

get\_default[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-get_default "Copy/link to this API element")

```python
get_default(self: Self, dest: str, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

register[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-register "Copy/link to this API element")

```python
register(self: Self, registry_name: Literal[str], value: str, object, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

parse\_args[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-parse_args "Copy/link to this API element")

```python
parse_args(self: Self, args = None, namespace = None, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

parse\_known\_args[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-parse_known_args "Copy/link to this API element")

```python
parse_known_args(self: Self, args = None, namespace = None, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

parse\_intermixed\_args[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-parse_intermixed_args "Copy/link to this API element")

```python
parse_intermixed_args(self: Self, args = None, namespace = None, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

parse\_known\_intermixed\_args[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-parse_known_intermixed_args "Copy/link to this API element")

```python
parse_known_intermixed_args(self: Self, args = None, namespace = None, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

convert\_arg\_line\_to\_args[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-convert_arg_line_to_args "Copy/link to this API element")

```python
convert_arg_line_to_args(self: Self, arg_line: str, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

exit[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-exit "Copy/link to this API element")

```python
exit(self: Self, status: int = 0, message: Optional[str] = None, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

error[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-error "Copy/link to this API element")

```python
error(self: Self, message: str, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

format\_usage[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-format_usage "Copy/link to this API element")

```python
format_usage(self: Self, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

format\_help[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-format_help "Copy/link to this API element")

```python
format_help(self: Self, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

print\_usage[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-print_usage "Copy/link to this API element")

```python
print_usage(self: Self, file = None, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

print\_help[](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/#class-argumentparser-print_help "Copy/link to this API element")

```python
print_help(self: Self, file = None, Self: type)
```

Defined in [`argparse`](https://docs.exaloop.io/libraries/api/modules/argparse/)

Source: https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/
