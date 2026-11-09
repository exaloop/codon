# argparse

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/argparse.codon)

Command-line parsing with a tagged `ArgumentValue` and a dynamic `Namespace`.

Parsed attributes are tagged values: use `args.count.as_int()`, `args.name.as_str()`, and `args.files.values()` to access native values.

## Custom Actions

Named action registration normalizes constructor arguments to the standard `Action` fields and does not forward additional constructor keywords. Pass the action class directly to supply custom keywords.

### Differences From CPython

-   **Positional arguments:** Custom actions are supported only for options. Using a custom class, factory, or registered action for a positional raises `TypeError` before construction.
-   **Initializer overloads:** Class construction checks only the leading `__init__` signature exposed by the compiler; it does not search additional overloads or base initializers. Use a single initializer, or a factory that explicitly calls the desired overload.
-   **Parser defaults:** For direct classes and factories, defaults from `set_defaults()` and `argument_default` are passed as `ArgumentValue`, not unboxed to a native constructor parameter type. Leave the default parameter unannotated or accept `ArgumentValue`; alternatively, pass `default=` directly to `add_argument()`, which preserves its native type.

Incompatible signatures are rejected at compile time when independent of parser defaults, and with `TypeError` when they depend on runtime parser defaults.

variable SUPPRESS[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-suppress "Copy/link to this API element")

```python
SUPPRESS: Literal[str] = "==SUPPRESS=="
```

variable OPTIONAL[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-optional "Copy/link to this API element")

```python
OPTIONAL: Literal[str] = "?"
```

variable ZERO\_OR\_MORE[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-zero_or_more "Copy/link to this API element")

```python
ZERO_OR_MORE: Literal[str] = "*"
```

variable ONE\_OR\_MORE[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-one_or_more "Copy/link to this API element")

```python
ONE_OR_MORE: Literal[str] = "+"
```

variable REMAINDER[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-remainder "Copy/link to this API element")

```python
REMAINDER: Literal[str] = "..."
```

variable PARSER[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-parser "Copy/link to this API element")

```python
PARSER: Literal[str] = "A..."
```

variable VALUE\_NULL[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-value_null "Copy/link to this API element")

```python
VALUE_NULL: Literal[int] = json._KIND_NULL
```

variable VALUE\_BOOL[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-value_bool "Copy/link to this API element")

```python
VALUE_BOOL: Literal[int] = json._KIND_BOOL
```

variable VALUE\_INT[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-value_int "Copy/link to this API element")

```python
VALUE_INT: Literal[int] = json._KIND_INT
```

variable VALUE\_FLOAT[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-value_float "Copy/link to this API element")

```python
VALUE_FLOAT: Literal[int] = json._KIND_FLOAT
```

variable VALUE\_STR[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-value_str "Copy/link to this API element")

```python
VALUE_STR: Literal[int] = json._KIND_STR
```

variable VALUE\_ARRAY[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-value_array "Copy/link to this API element")

```python
VALUE_ARRAY: Literal[int] = json._KIND_ARRAY
```

variable VALUE\_OBJECT[](https://docs.exaloop.io/libraries/api/modules/argparse/#variable-value_object "Copy/link to this API element")

```python
VALUE_OBJECT: Literal[int] = 7
```

class ArgumentValue[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue "Copy/link to this API element")

@tuple

```python
class ArgumentValue
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentValue/)

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-__new__ "Copy/link to this API element")

```python
__new__()
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-__new__-overload-2 "Copy/link to this API element")

```python
__new__(value)
```

Properties 1

kind[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-kind "Copy/link to this API element")

@property

```python
kind(self: Self, Self: type)
```

Methods 9

array[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-array "Copy/link to this API element")

```python
array()
```

is\_null[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-is_null "Copy/link to this API element")

```python
is_null(self: Self, Self: type)
```

as\_str[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-as_str "Copy/link to this API element")

```python
as_str(self: Self, Self: type)
```

as\_int[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-as_int "Copy/link to this API element")

```python
as_int(self: Self, Self: type)
```

as\_float[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-as_float "Copy/link to this API element")

```python
as_float(self: Self, Self: type)
```

as\_bool[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-as_bool "Copy/link to this API element")

```python
as_bool(self: Self, Self: type)
```

as\_type[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-as_type "Copy/link to this API element")

```python
as_type(self: Self, T: type, Self: type)
```

values[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-values "Copy/link to this API element")

```python
values(self: Self, Self: type)
```

append[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-append "Copy/link to this API element")

```python
append(self: Self, value, Self: type)
```

Special methods 7

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int, Self: type)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentvalue-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

class Namespace[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-namespace "Copy/link to this API element")

```python
class Namespace
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/Namespace/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-namespace-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs, Self: type)
```

Special methods 9

\_\_getattr\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-namespace-__getattr__ "Copy/link to this API element")

```python
__getattr__(self: Self, name: Literal[str], Self: type)
```

\_\_setattr\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-namespace-__setattr__ "Copy/link to this API element")

```python
__setattr__(self: Self, name: Literal[str], value, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-namespace-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, name: str, Self: type)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-namespace-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, name: str, value, Self: type)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-namespace-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, name: str, Self: type)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-namespace-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, name: str, Self: type)
```

\_\_dict\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-namespace-__dict__ "Copy/link to this API element")

```python
__dict__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-namespace-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: Namespace, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-namespace-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

class ArgumentTypeError[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumenttypeerror "Copy/link to this API element")

```python
class ArgumentTypeError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentTypeError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumenttypeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str, Self: type)
```

class ArgumentError[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumenterror "Copy/link to this API element")

```python
class ArgumentError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentError/)

Fields 1

`argument_name` `str`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumenterror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, argument, message: str, Self: type)
```

Special methods 1

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumenterror-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

class Action[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-action "Copy/link to this API element")

```python
class Action
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/Action/)

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

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-action-__init__ "Copy/link to this API element")

```python
__init__(self: Self, option_strings, dest: str, nargs = None, const = None, default = None, choices = None, required: bool = False, help: Optional[str] = None, metavar = None, deprecated: bool = False, **kwargs, Self: type)
```

Methods 1

format\_usage[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-action-format_usage "Copy/link to this API element")

```python
format_usage(self: Self, Self: type)
```

Special methods 2

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-action-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: Action, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-action-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: Action, Self: type)
```

class BooleanOptionalAction[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-booleanoptionalaction "Copy/link to this API element")

```python
class BooleanOptionalAction
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/BooleanOptionalAction/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-booleanoptionalaction-__init__ "Copy/link to this API element")

```python
__init__(self: Self, option_strings, dest: str, default = None, choices = None, required: bool = False, help: Optional[str] = None, metavar = None, deprecated: bool = False, **kwargs, Self: type)
```

class HelpFormatter[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-helpformatter "Copy/link to this API element")

```python
class HelpFormatter
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/HelpFormatter/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-helpformatter-__init__ "Copy/link to this API element")

```python
__init__(self: Self, prog: str, indent_increment: int = 2, max_help_position: int = 24, width: Optional[int] = None, color: bool = True, Self: type)
```

Methods 7

start\_section[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-helpformatter-start_section "Copy/link to this API element")

```python
start_section(self: Self, heading: Optional[str], Self: type)
```

end\_section[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-helpformatter-end_section "Copy/link to this API element")

```python
end_section(self: Self, Self: type)
```

add\_text[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-helpformatter-add_text "Copy/link to this API element")

```python
add_text(self: Self, text: Optional[str], Self: type)
```

add\_usage[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-helpformatter-add_usage "Copy/link to this API element")

```python
add_usage(self: Self, usage: Optional[str], actions: List[Action], groups: List[_MutuallyExclusiveGroup], prefix: Optional[str] = None, Self: type)
```

add\_argument[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-helpformatter-add_argument "Copy/link to this API element")

```python
add_argument(self: Self, action: Action, Self: type)
```

add\_arguments[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-helpformatter-add_arguments "Copy/link to this API element")

```python
add_arguments(self: Self, actions: List[Action], Self: type)
```

format\_help[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-helpformatter-format_help "Copy/link to this API element")

```python
format_help(self: Self, Self: type)
```

class RawDescriptionHelpFormatter[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-rawdescriptionhelpformatter "Copy/link to this API element")

```python
class RawDescriptionHelpFormatter
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/RawDescriptionHelpFormatter/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-rawdescriptionhelpformatter-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class RawTextHelpFormatter[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-rawtexthelpformatter "Copy/link to this API element")

```python
class RawTextHelpFormatter
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/RawTextHelpFormatter/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-rawtexthelpformatter-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class ArgumentDefaultsHelpFormatter[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentdefaultshelpformatter "Copy/link to this API element")

```python
class ArgumentDefaultsHelpFormatter
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentDefaultsHelpFormatter/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentdefaultshelpformatter-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class MetavarTypeHelpFormatter[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-metavartypehelpformatter "Copy/link to this API element")

```python
class MetavarTypeHelpFormatter
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/MetavarTypeHelpFormatter/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-metavartypehelpformatter-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class FileType[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-filetype "Copy/link to this API element")

```python
class FileType
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/FileType/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-filetype-__init__ "Copy/link to this API element")

```python
__init__(self: Self, mode: str = "r", bufsize: int = -1, encoding: Optional[str] = None, errors: Optional[str] = None, Self: type)
```

Special methods 2

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-filetype-__call__ "Copy/link to this API element")

```python
__call__(self: Self, string: str, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-filetype-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

class ArgumentParser[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser "Copy/link to this API element")

```python
class ArgumentParser
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/ArgumentParser/)

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

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-__init__ "Copy/link to this API element")

```python
__init__(self: Self, prog: Optional[str] = None, usage: Optional[str] = None, description: Optional[str] = None, epilog: Optional[str] = None, parents = None, formatter_class = HelpFormatter, prefix_chars: str = "-", fromfile_prefix_chars: Optional[str] = None, argument_default = None, conflict_handler: str = "error", add_help: bool = True, allow_abbrev: bool = True, exit_on_error: bool = True, suggest_on_error: bool = False, color: bool = True, Self: type)
```

Methods 18

add\_argument[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-add_argument "Copy/link to this API element")

```python
add_argument(self: Self, *names, **kwargs, Self: type)
```

add\_argument\_group[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-add_argument_group "Copy/link to this API element")

```python
add_argument_group(self: Self, title: str = "", description: Optional[str] = None, Self: type)
```

add\_mutually\_exclusive\_group[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-add_mutually_exclusive_group "Copy/link to this API element")

```python
add_mutually_exclusive_group(self: Self, required: bool = False, Self: type)
```

add\_subparsers[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-add_subparsers "Copy/link to this API element")

```python
add_subparsers(self: Self, title: Optional[str] = None, description: Optional[str] = None, prog: Optional[str] = None, parser_class = None, dest: str = SUPPRESS, required: bool = False, help: Optional[str] = None, metavar = None, Self: type)
```

set\_defaults[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-set_defaults "Copy/link to this API element")

```python
set_defaults(self: Self, **kwargs, Self: type)
```

get\_default[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-get_default "Copy/link to this API element")

```python
get_default(self: Self, dest: str, Self: type)
```

register[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-register "Copy/link to this API element")

```python
register(self: Self, registry_name: Literal[str], value: str, object, Self: type)
```

parse\_args[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-parse_args "Copy/link to this API element")

```python
parse_args(self: Self, args = None, namespace = None, Self: type)
```

parse\_known\_args[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-parse_known_args "Copy/link to this API element")

```python
parse_known_args(self: Self, args = None, namespace = None, Self: type)
```

parse\_intermixed\_args[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-parse_intermixed_args "Copy/link to this API element")

```python
parse_intermixed_args(self: Self, args = None, namespace = None, Self: type)
```

parse\_known\_intermixed\_args[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-parse_known_intermixed_args "Copy/link to this API element")

```python
parse_known_intermixed_args(self: Self, args = None, namespace = None, Self: type)
```

convert\_arg\_line\_to\_args[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-convert_arg_line_to_args "Copy/link to this API element")

```python
convert_arg_line_to_args(self: Self, arg_line: str, Self: type)
```

exit[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-exit "Copy/link to this API element")

```python
exit(self: Self, status: int = 0, message: Optional[str] = None, Self: type)
```

error[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-error "Copy/link to this API element")

```python
error(self: Self, message: str, Self: type)
```

format\_usage[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-format_usage "Copy/link to this API element")

```python
format_usage(self: Self, Self: type)
```

format\_help[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-format_help "Copy/link to this API element")

```python
format_help(self: Self, Self: type)
```

print\_usage[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-print_usage "Copy/link to this API element")

```python
print_usage(self: Self, file = None, Self: type)
```

print\_help[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-argumentparser-print_help "Copy/link to this API element")

```python
print_help(self: Self, file = None, Self: type)
```

class Action[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-action "Copy/link to this API element")

@extend

```python
class Action
```

Extends [`argparse.Action`](https://docs.exaloop.io/libraries/api/classes/argparse/Action/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/Action/)

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-action-__call__ "Copy/link to this API element")

```python
__call__(self: Self, parser, namespace: Namespace, values: ArgumentValue, option_string: Optional[str] = None, Self: type)
```

class BooleanOptionalAction[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-booleanoptionalaction "Copy/link to this API element")

@extend

```python
class BooleanOptionalAction
```

Extends [`argparse.BooleanOptionalAction`](https://docs.exaloop.io/libraries/api/classes/argparse/BooleanOptionalAction/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/argparse/BooleanOptionalAction/)

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/argparse/#class-booleanoptionalaction-__call__ "Copy/link to this API element")

```python
__call__(self: Self, parser: ArgumentParser, namespace: Namespace, values: ArgumentValue, option_string: Optional[str] = None, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/argparse/
