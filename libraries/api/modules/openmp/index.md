# openmp

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/openmp.codon)

variable Routine[](https://docs.exaloop.io/libraries/api/modules/openmp/#variable-routine "Copy/link to this API element")

```python
Routine = Function[([i32, cobj], i32)]
```

class Lock[](https://docs.exaloop.io/libraries/api/modules/openmp/#class-lock "Copy/link to this API element")

@tuple

```python
class Lock
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/openmp/Lock/)

Fields 8

`a1` `i32`

`a2` `i32`

`a3` `i32`

`a4` `i32`

`a5` `i32`

`a6` `i32`

`a7` `i32`

`a8` `i32`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/openmp/#class-lock-__new__ "Copy/link to this API element")

```python
__new__()
```

class Ident[](https://docs.exaloop.io/libraries/api/modules/openmp/#class-ident "Copy/link to this API element")

@tuple

```python
class Ident
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/openmp/Ident/)

Fields 5

`reserved_1` `i32`

`flags` `i32`

`reserved_2` `i32`

`reserved_3` `i32`

`psource` `cobj`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/openmp/#class-ident-__new__ "Copy/link to this API element")

```python
__new__(flags: int = 0, source: str = ";unknown;unknown;0;0;;")
```

class LRData[](https://docs.exaloop.io/libraries/api/modules/openmp/#class-lrdata "Copy/link to this API element")

@tuple

```python
class LRData
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/openmp/LRData/)

Fields 1

`routine` `Routine`

class Task[](https://docs.exaloop.io/libraries/api/modules/openmp/#class-task "Copy/link to this API element")

@tuple

```python
class Task
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/openmp/Task/)

Fields 5

`shareds` `cobj`

`routine` `Routine`

`flags` `i32`

`x` `LRData`

`y` `LRData`

class TaskWithPrivates[](https://docs.exaloop.io/libraries/api/modules/openmp/#class-taskwithprivates "Copy/link to this API element")

@tuple

```python
class TaskWithPrivates[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/openmp/TaskWithPrivates/)

Fields 3

`task` `Task`

`data` `T`

`T` `type`

class TaskReductionInput[](https://docs.exaloop.io/libraries/api/modules/openmp/#class-taskreductioninput "Copy/link to this API element")

@tuple

```python
class TaskReductionInput
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/openmp/TaskReductionInput/)

Fields 7

`reduce_shar` `cobj`

`reduce_orig` `cobj`

`reduce_size` `int`

`reduce_init` `cobj`

`reduce_fini` `cobj`

`reduce_comb` `cobj`

`flags` `u32`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/openmp/#class-taskreductioninput-__new__ "Copy/link to this API element")

```python
__new__(reduce_shar, reduce_orig, reduce_size: int, reduce_init: cobj, reduce_comb: cobj)
```

function flush[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-flush "Copy/link to this API element")

```python
flush()
```

function get\_num\_threads[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_num_threads "Copy/link to this API element")

@pure

```python
get_num_threads()
```

function get\_thread\_num[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_thread_num "Copy/link to this API element")

@pure

```python
get_thread_num()
```

function get\_max\_threads[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_max_threads "Copy/link to this API element")

@pure

```python
get_max_threads()
```

function get\_num\_procs[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_num_procs "Copy/link to this API element")

@pure

```python
get_num_procs()
```

function set\_num\_threads[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-set_num_threads "Copy/link to this API element")

```python
set_num_threads(num_threads: int)
```

function in\_parallel[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-in_parallel "Copy/link to this API element")

@pure

```python
in_parallel()
```

function set\_dynamic[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-set_dynamic "Copy/link to this API element")

```python
set_dynamic(dynamic_threads: bool = True)
```

function get\_dynamic[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_dynamic "Copy/link to this API element")

@pure

```python
get_dynamic()
```

function get\_cancellation[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_cancellation "Copy/link to this API element")

@pure

```python
get_cancellation()
```

function set\_schedule[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-set_schedule "Copy/link to this API element")

```python
set_schedule(kind: str, chunk_size: int = 0)
```

function get\_schedule[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_schedule "Copy/link to this API element")

@pure

```python
get_schedule()
```

function get\_thread\_limit[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_thread_limit "Copy/link to this API element")

@pure

```python
get_thread_limit()
```

function set\_max\_active\_levels[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-set_max_active_levels "Copy/link to this API element")

```python
set_max_active_levels(max_levels: int)
```

function get\_max\_active\_levels[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_max_active_levels "Copy/link to this API element")

@pure

```python
get_max_active_levels()
```

function get\_level[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_level "Copy/link to this API element")

@pure

```python
get_level()
```

function get\_ancestor\_thread\_num[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_ancestor_thread_num "Copy/link to this API element")

@pure

```python
get_ancestor_thread_num(level: int)
```

function get\_team\_size[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_team_size "Copy/link to this API element")

@pure

```python
get_team_size(level: int)
```

function get\_active\_level[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_active_level "Copy/link to this API element")

@pure

```python
get_active_level()
```

function in\_final[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-in_final "Copy/link to this API element")

@pure

```python
in_final()
```

function get\_proc\_bind[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_proc_bind "Copy/link to this API element")

@pure

```python
get_proc_bind()
```

function set\_default\_device[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-set_default_device "Copy/link to this API element")

```python
set_default_device(device_num: int)
```

function get\_default\_device[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_default_device "Copy/link to this API element")

@pure

```python
get_default_device()
```

function get\_num\_devices[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_num_devices "Copy/link to this API element")

@pure

```python
get_num_devices()
```

function get\_num\_teams[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_num_teams "Copy/link to this API element")

@pure

```python
get_num_teams()
```

function get\_team\_num[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_team_num "Copy/link to this API element")

@pure

```python
get_team_num()
```

function is\_initial\_device[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-is_initial_device "Copy/link to this API element")

@pure

```python
is_initial_device()
```

function get\_wtime[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_wtime "Copy/link to this API element")

@pure

```python
get_wtime()
```

function get\_wtick[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-get_wtick "Copy/link to this API element")

@pure

```python
get_wtick()
```

function single[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-single "Copy/link to this API element")

```python
single(func)
```

function master[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-master "Copy/link to this API element")

```python
master(func)
```

function ordered[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-ordered "Copy/link to this API element")

```python
ordered(func)
```

function critical[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-critical "Copy/link to this API element")

```python
critical(func)
```

function for\_par[](https://docs.exaloop.io/libraries/api/modules/openmp/#function-for_par "Copy/link to this API element")

```python
for_par(num_threads: int = -1, chunk_size: int = 0, schedule: Literal[str] = "static", ordered: Literal[bool] = False, collapse: Literal[int] = 0, gpu: Literal[bool] = False)
```

Source: https://docs.exaloop.io/libraries/api/modules/openmp/
