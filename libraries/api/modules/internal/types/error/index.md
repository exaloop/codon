# internal.types.error

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/error.codon)

class BaseException[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-baseexception "Copy/link to this API element")

```python
class BaseException
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/BaseException/)

Fields 7

`message` `str`

`func` `str`

`file` `str`

`line` `int`

`col` `int`

`python_type` `cobj`

`cause` `Optional[BaseException]`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-baseexception-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

Properties 1

typename[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-baseexception-typename "Copy/link to this API element")

@property

```python
typename(self: Self, Self: type)
```

Special methods 3

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-baseexception-__str__ "Copy/link to this API element")

```python
__str__(self: BaseException)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-baseexception-__repr__ "Copy/link to this API element")

```python
__repr__(self: BaseException)
```

\_\_cause\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-baseexception-__cause__ "Copy/link to this API element")

@property

```python
__cause__(self: Self, Self: type)
```

class Exception[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-exception "Copy/link to this API element")

```python
class Exception
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/Exception/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-exception-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class NameError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-nameerror "Copy/link to this API element")

```python
class NameError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/NameError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-nameerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class OSError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-oserror "Copy/link to this API element")

```python
class OSError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/OSError/)

Fields 5

`errno` `Optional[int]`

`strerror` `Optional[str]`

`filename` `Optional[str]`

`filename2` `Optional[str]`

`winerror` `Optional[int]`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-oserror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-oserror-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, errno: Optional[int], strerror: str, filename: Optional[str] = None, winerror: Optional[int] = None, filename2: Optional[str] = None, Self: type)
```

class ValueError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-valueerror "Copy/link to this API element")

```python
class ValueError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ValueError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-valueerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class LookupError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-lookuperror "Copy/link to this API element")

```python
class LookupError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/LookupError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-lookuperror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class IndexError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-indexerror "Copy/link to this API element")

```python
class IndexError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/IndexError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-indexerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class KeyError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-keyerror "Copy/link to this API element")

```python
class KeyError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/KeyError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-keyerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class CError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-cerror "Copy/link to this API element")

```python
class CError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/CError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-cerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class TypeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-typeerror "Copy/link to this API element")

```python
class TypeError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/TypeError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-typeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class ArithmeticError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-arithmeticerror "Copy/link to this API element")

```python
class ArithmeticError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ArithmeticError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-arithmeticerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class ZeroDivisionError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-zerodivisionerror "Copy/link to this API element")

```python
class ZeroDivisionError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ZeroDivisionError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-zerodivisionerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class OverflowError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-overflowerror "Copy/link to this API element")

```python
class OverflowError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/OverflowError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-overflowerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class AttributeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-attributeerror "Copy/link to this API element")

```python
class AttributeError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/AttributeError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-attributeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class RuntimeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-runtimeerror "Copy/link to this API element")

```python
class RuntimeError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/RuntimeError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-runtimeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class NotImplementedError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-notimplementederror "Copy/link to this API element")

```python
class NotImplementedError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/NotImplementedError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-notimplementederror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class StopIteration[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-stopiteration "Copy/link to this API element")

```python
class StopIteration
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/StopIteration/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-stopiteration-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class AssertionError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-assertionerror "Copy/link to this API element")

```python
class AssertionError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/AssertionError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-assertionerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class EOFError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-eoferror "Copy/link to this API element")

```python
class EOFError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/EOFError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-eoferror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class SystemExit[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemexit "Copy/link to this API element")

```python
class SystemExit
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/SystemExit/)

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemexit-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str, status: int = 1, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemexit-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, status: int = 0, Self: type)
```

Properties 1

status[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemexit-status "Copy/link to this API element")

@property

```python
status(self: Self, Self: type)
```

class StaticCompileError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-staticcompileerror "Copy/link to this API element")

```python
class StaticCompileError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/StaticCompileError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-staticcompileerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class GeneratorExit[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-generatorexit "Copy/link to this API element")

```python
class GeneratorExit
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/GeneratorExit/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-generatorexit-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class KeyboardInterrupt[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-keyboardinterrupt "Copy/link to this API element")

```python
class KeyboardInterrupt
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/KeyboardInterrupt/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-keyboardinterrupt-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class MemoryError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-memoryerror "Copy/link to this API element")

```python
class MemoryError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/MemoryError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-memoryerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class ReferenceError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-referenceerror "Copy/link to this API element")

```python
class ReferenceError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ReferenceError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-referenceerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class SystemError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemerror "Copy/link to this API element")

```python
class SystemError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/SystemError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class StopAsyncIteration[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-stopasynciteration "Copy/link to this API element")

```python
class StopAsyncIteration
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/StopAsyncIteration/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-stopasynciteration-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class FloatingPointError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-floatingpointerror "Copy/link to this API element")

```python
class FloatingPointError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/FloatingPointError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-floatingpointerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class RecursionError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-recursionerror "Copy/link to this API element")

```python
class RecursionError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/RecursionError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-recursionerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class UnicodeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeerror "Copy/link to this API element")

```python
class UnicodeError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/UnicodeError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class UnicodeDecodeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodedecodeerror "Copy/link to this API element")

```python
class UnicodeDecodeError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/UnicodeDecodeError/)

Fields 5

`encoding` `str`

`object` `bytes`

`start` `int`

`end` `int`

`reason` `str`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodedecodeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, encoding: str, object: bytes, start: int, end: int, reason: str, Self: type)
```

Properties 1

args[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodedecodeerror-args "Copy/link to this API element")

@property

```python
args(self: Self, Self: type)
```

Special methods 2

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodedecodeerror-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodedecodeerror-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

class UnicodeEncodeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeencodeerror "Copy/link to this API element")

```python
class UnicodeEncodeError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/UnicodeEncodeError/)

Fields 5

`encoding` `str`

`object` `str`

`start` `int`

`end` `int`

`reason` `str`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeencodeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, encoding: str, object: str, start: int, end: int, reason: str, Self: type)
```

Properties 1

args[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeencodeerror-args "Copy/link to this API element")

@property

```python
args(self: Self, Self: type)
```

Special methods 2

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeencodeerror-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeencodeerror-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

class UnicodeTranslateError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodetranslateerror "Copy/link to this API element")

```python
class UnicodeTranslateError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/UnicodeTranslateError/)

Fields 4

`object` `str`

`start` `int`

`end` `int`

`reason` `str`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodetranslateerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, object: str, start: int, end: int, reason: str, Self: type)
```

Properties 2

encoding[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodetranslateerror-encoding "Copy/link to this API element")

@property

```python
encoding(self: Self, Self: type)
```

args[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodetranslateerror-args "Copy/link to this API element")

@property

```python
args(self: Self, Self: type)
```

Special methods 2

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodetranslateerror-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodetranslateerror-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

class BlockingIOError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-blockingioerror "Copy/link to this API element")

```python
class BlockingIOError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/BlockingIOError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-blockingioerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class ChildProcessError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-childprocesserror "Copy/link to this API element")

```python
class ChildProcessError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ChildProcessError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-childprocesserror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class ConnectionError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionerror "Copy/link to this API element")

```python
class ConnectionError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ConnectionError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class BrokenPipeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-brokenpipeerror "Copy/link to this API element")

```python
class BrokenPipeError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/BrokenPipeError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-brokenpipeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class ConnectionAbortedError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionabortederror "Copy/link to this API element")

```python
class ConnectionAbortedError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ConnectionAbortedError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionabortederror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class ConnectionRefusedError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionrefusederror "Copy/link to this API element")

```python
class ConnectionRefusedError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ConnectionRefusedError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionrefusederror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class ConnectionResetError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionreseterror "Copy/link to this API element")

```python
class ConnectionResetError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ConnectionResetError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionreseterror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class FileExistsError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-fileexistserror "Copy/link to this API element")

```python
class FileExistsError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/FileExistsError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-fileexistserror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class FileNotFoundError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-filenotfounderror "Copy/link to this API element")

```python
class FileNotFoundError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/FileNotFoundError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-filenotfounderror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class InterruptedError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-interruptederror "Copy/link to this API element")

```python
class InterruptedError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/InterruptedError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-interruptederror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class IsADirectoryError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-isadirectoryerror "Copy/link to this API element")

```python
class IsADirectoryError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/IsADirectoryError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-isadirectoryerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class NotADirectoryError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-notadirectoryerror "Copy/link to this API element")

```python
class NotADirectoryError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/NotADirectoryError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-notadirectoryerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class PermissionError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-permissionerror "Copy/link to this API element")

```python
class PermissionError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/PermissionError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-permissionerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class ProcessLookupError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-processlookuperror "Copy/link to this API element")

```python
class ProcessLookupError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ProcessLookupError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-processlookuperror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

class TimeoutError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-timeouterror "Copy/link to this API element")

```python
class TimeoutError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/TimeoutError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-timeouterror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *args, **kwargs, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/error/
