# internal.types.error

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/error.codon)

class BaseException[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-baseexception "Copy/link to this API element")

```python
class BaseException(object)
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
__init__(self: Self, message: str = "", /)
```

Properties 1

typename[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-baseexception-typename "Copy/link to this API element")

@property

```python
typename(self: Self)
```

Special methods 3

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-baseexception-__str__ "Copy/link to this API element")

```python
__str__(self: BaseException, /) -> str
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-baseexception-__repr__ "Copy/link to this API element")

```python
__repr__(self: BaseException, /) -> str
```

\_\_cause\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-baseexception-__cause__ "Copy/link to this API element")

@property

```python
__cause__(self: Self)
```

class Exception[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-exception "Copy/link to this API element")

```python
class Exception(BaseException)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/Exception/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-exception-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class NameError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-nameerror "Copy/link to this API element")

```python
class NameError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/NameError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-nameerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class OSError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-oserror "Copy/link to this API element")

```python
class OSError(Exception)
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
__init__(self: Self, message: str = "", /)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-oserror-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, errno: Optional[int], strerror: str, filename: Optional[str] = None, winerror: Optional[int] = None, filename2: Optional[str] = None, /)
```

alias IOError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#alias-ioerror "Copy/link to this API element")

```python
IOError = OSError
```

Alias of [`OSError`](https://docs.exaloop.io/libraries/api/classes/internal/types/error/OSError/)

class ValueError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-valueerror "Copy/link to this API element")

```python
class ValueError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ValueError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-valueerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class LookupError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-lookuperror "Copy/link to this API element")

```python
class LookupError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/LookupError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-lookuperror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class IndexError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-indexerror "Copy/link to this API element")

```python
class IndexError(LookupError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/IndexError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-indexerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class KeyError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-keyerror "Copy/link to this API element")

```python
class KeyError(LookupError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/KeyError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-keyerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class CError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-cerror "Copy/link to this API element")

```python
class CError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/CError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-cerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "")
```

class TypeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-typeerror "Copy/link to this API element")

```python
class TypeError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/TypeError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-typeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class ArithmeticError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-arithmeticerror "Copy/link to this API element")

```python
class ArithmeticError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ArithmeticError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-arithmeticerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class ZeroDivisionError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-zerodivisionerror "Copy/link to this API element")

```python
class ZeroDivisionError(ArithmeticError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ZeroDivisionError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-zerodivisionerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class OverflowError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-overflowerror "Copy/link to this API element")

```python
class OverflowError(ArithmeticError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/OverflowError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-overflowerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class AttributeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-attributeerror "Copy/link to this API element")

```python
class AttributeError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/AttributeError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-attributeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class RuntimeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-runtimeerror "Copy/link to this API element")

```python
class RuntimeError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/RuntimeError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-runtimeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class NotImplementedError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-notimplementederror "Copy/link to this API element")

```python
class NotImplementedError(RuntimeError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/NotImplementedError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-notimplementederror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class StopIteration[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-stopiteration "Copy/link to this API element")

```python
class StopIteration(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/StopIteration/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-stopiteration-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class AssertionError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-assertionerror "Copy/link to this API element")

```python
class AssertionError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/AssertionError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-assertionerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class EOFError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-eoferror "Copy/link to this API element")

```python
class EOFError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/EOFError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-eoferror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class SystemExit[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemexit "Copy/link to this API element")

```python
class SystemExit(BaseException)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/SystemExit/)

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemexit-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str, status: int = 1, /)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemexit-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, status: int = 0, /)
```

Properties 1

status[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemexit-status "Copy/link to this API element")

@property

```python
status(self: Self)
```

class StaticCompileError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-staticcompileerror "Copy/link to this API element")

```python
class StaticCompileError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/StaticCompileError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-staticcompileerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "")
```

alias EnvironmentError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#alias-environmenterror "Copy/link to this API element")

```python
EnvironmentError = OSError
```

Alias of [`OSError`](https://docs.exaloop.io/libraries/api/classes/internal/types/error/OSError/)

class GeneratorExit[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-generatorexit "Copy/link to this API element")

```python
class GeneratorExit(BaseException)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/GeneratorExit/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-generatorexit-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class KeyboardInterrupt[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-keyboardinterrupt "Copy/link to this API element")

```python
class KeyboardInterrupt(BaseException)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/KeyboardInterrupt/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-keyboardinterrupt-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class MemoryError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-memoryerror "Copy/link to this API element")

```python
class MemoryError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/MemoryError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-memoryerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class ReferenceError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-referenceerror "Copy/link to this API element")

```python
class ReferenceError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ReferenceError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-referenceerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class SystemError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemerror "Copy/link to this API element")

```python
class SystemError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/SystemError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-systemerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class StopAsyncIteration[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-stopasynciteration "Copy/link to this API element")

```python
class StopAsyncIteration(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/StopAsyncIteration/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-stopasynciteration-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class FloatingPointError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-floatingpointerror "Copy/link to this API element")

```python
class FloatingPointError(ArithmeticError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/FloatingPointError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-floatingpointerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class RecursionError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-recursionerror "Copy/link to this API element")

```python
class RecursionError(RuntimeError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/RecursionError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-recursionerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class UnicodeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeerror "Copy/link to this API element")

```python
class UnicodeError(ValueError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/UnicodeError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class UnicodeDecodeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodedecodeerror "Copy/link to this API element")

```python
class UnicodeDecodeError(UnicodeError)
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
__init__(self: Self, encoding: str, object: bytes, start: int, end: int, reason: str, /)
```

Properties 1

args[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodedecodeerror-args "Copy/link to this API element")

@property

```python
args(self: Self)
```

Special methods 2

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodedecodeerror-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodedecodeerror-__str__ "Copy/link to this API element")

```python
__str__(self: Self, /)
```

class UnicodeEncodeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeencodeerror "Copy/link to this API element")

```python
class UnicodeEncodeError(UnicodeError)
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
__init__(self: Self, encoding: str, object: str, start: int, end: int, reason: str, /)
```

Properties 1

args[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeencodeerror-args "Copy/link to this API element")

@property

```python
args(self: Self)
```

Special methods 2

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeencodeerror-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodeencodeerror-__str__ "Copy/link to this API element")

```python
__str__(self: Self, /)
```

class UnicodeTranslateError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodetranslateerror "Copy/link to this API element")

```python
class UnicodeTranslateError(UnicodeError)
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
__init__(self: Self, object: str, start: int, end: int, reason: str, /)
```

Properties 2

encoding[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodetranslateerror-encoding "Copy/link to this API element")

@property

```python
encoding(self: Self) -> Optional[str]
```

args[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodetranslateerror-args "Copy/link to this API element")

@property

```python
args(self: Self)
```

Special methods 2

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodetranslateerror-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-unicodetranslateerror-__str__ "Copy/link to this API element")

```python
__str__(self: Self, /)
```

class BlockingIOError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-blockingioerror "Copy/link to this API element")

```python
class BlockingIOError(OSError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/BlockingIOError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-blockingioerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class ChildProcessError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-childprocesserror "Copy/link to this API element")

```python
class ChildProcessError(OSError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ChildProcessError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-childprocesserror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class ConnectionError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionerror "Copy/link to this API element")

```python
class ConnectionError(OSError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ConnectionError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class BrokenPipeError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-brokenpipeerror "Copy/link to this API element")

```python
class BrokenPipeError(ConnectionError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/BrokenPipeError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-brokenpipeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class ConnectionAbortedError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionabortederror "Copy/link to this API element")

```python
class ConnectionAbortedError(ConnectionError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ConnectionAbortedError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionabortederror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class ConnectionRefusedError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionrefusederror "Copy/link to this API element")

```python
class ConnectionRefusedError(ConnectionError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ConnectionRefusedError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionrefusederror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class ConnectionResetError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionreseterror "Copy/link to this API element")

```python
class ConnectionResetError(ConnectionError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ConnectionResetError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-connectionreseterror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class FileExistsError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-fileexistserror "Copy/link to this API element")

```python
class FileExistsError(OSError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/FileExistsError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-fileexistserror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class FileNotFoundError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-filenotfounderror "Copy/link to this API element")

```python
class FileNotFoundError(OSError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/FileNotFoundError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-filenotfounderror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class InterruptedError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-interruptederror "Copy/link to this API element")

```python
class InterruptedError(OSError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/InterruptedError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-interruptederror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class IsADirectoryError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-isadirectoryerror "Copy/link to this API element")

```python
class IsADirectoryError(OSError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/IsADirectoryError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-isadirectoryerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class NotADirectoryError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-notadirectoryerror "Copy/link to this API element")

```python
class NotADirectoryError(OSError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/NotADirectoryError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-notadirectoryerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class PermissionError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-permissionerror "Copy/link to this API element")

```python
class PermissionError(OSError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/PermissionError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-permissionerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class ProcessLookupError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-processlookuperror "Copy/link to this API element")

```python
class ProcessLookupError(OSError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/ProcessLookupError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-processlookuperror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

class TimeoutError[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-timeouterror "Copy/link to this API element")

```python
class TimeoutError(OSError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/error/TimeoutError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/error/#class-timeouterror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *args, **kwargs)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/error/
