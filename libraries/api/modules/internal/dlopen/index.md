# internal.dlopen

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/dlopen.codon)

class DL\_info[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#class-dl_info "Copy/link to this API element")

@tuple

```python
class DL_info
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/dlopen/DL_info/)

Fields 7

`dli_fname` `cobj`

`dli_fbase` `cobj`

`dli_sname` `cobj`

`dli_saddr` `cobj`

`dli_version` `i32`

`dli_reserved1` `i32`

`dli_reserved` `Ptr[i32]`

function c\_dlerror[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#function-c_dlerror "Copy/link to this API element")

@C

```python
c_dlerror()
```

function c\_dlopen[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#function-c_dlopen "Copy/link to this API element")

@C

```python
c_dlopen(_: cobj, _: int)
```

function c\_dlsym[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#function-c_dlsym "Copy/link to this API element")

@C

```python
c_dlsym(_: cobj, _: cobj)
```

function c\_dlclose[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#function-c_dlclose "Copy/link to this API element")

@C

```python
c_dlclose(_: cobj)
```

function dladdr[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#function-dladdr "Copy/link to this API element")

@C

```python
dladdr(_: cobj, _: Ptr[DL_info])
```

variable RTLD\_LAZY[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#variable-rtld_lazy "Copy/link to this API element")

```python
RTLD_LAZY: Literal[int] = 1
```

variable RTLD\_NOW[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#variable-rtld_now "Copy/link to this API element")

```python
RTLD_NOW: Literal[int] = 2
```

variable RTLD\_GLOBAL[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#variable-rtld_global "Copy/link to this API element")

```python
RTLD_GLOBAL: Literal[int] = (8 if __apple__ else 256)
```

variable RTLD\_LOCAL[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#variable-rtld_local "Copy/link to this API element")

```python
RTLD_LOCAL: Literal[int] = (0 if __apple__ else 256)
```

function dlext[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#function-dlext "Copy/link to this API element")

```python
dlext()
```

function dlerror[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#function-dlerror "Copy/link to this API element")

@pure

```python
dlerror()
```

function dlopen[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#function-dlopen "Copy/link to this API element")

```python
dlopen(name: str, flag: int = (RTLD_NOW | RTLD_GLOBAL))
```

function dlsym[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#function-dlsym "Copy/link to this API element")

```python
dlsym(lib, name: str, Fn: type)
```

function dlclose[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#function-dlclose "Copy/link to this API element")

```python
dlclose(handle: cobj)
```

function get\_library\_file\_name[](https://docs.exaloop.io/libraries/api/modules/internal/dlopen/#function-get_library_file_name "Copy/link to this API element")

```python
get_library_file_name(handle: cobj)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/dlopen/
