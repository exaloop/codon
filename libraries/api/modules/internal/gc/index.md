# internal.gc

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/gc.codon)

function seq\_alloc[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-seq_alloc "Copy/link to this API element")

@pure @C

```python
seq_alloc(a: int)
```

function seq\_alloc\_atomic[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-seq_alloc_atomic "Copy/link to this API element")

@pure @C

```python
seq_alloc_atomic(a: int)
```

function seq\_alloc\_uncollectable[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-seq_alloc_uncollectable "Copy/link to this API element")

@pure @C

```python
seq_alloc_uncollectable(a: int)
```

function seq\_alloc\_atomic\_uncollectable[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-seq_alloc_atomic_uncollectable "Copy/link to this API element")

@pure @C

```python
seq_alloc_atomic_uncollectable(a: int)
```

function seq\_realloc[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-seq_realloc "Copy/link to this API element")

@nocapture @derives @C

```python
seq_realloc(p: cobj, newsize: int, oldsize: int)
```

function seq\_free[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-seq_free "Copy/link to this API element")

@nocapture @C

```python
seq_free(p: cobj)
```

function seq\_register\_finalizer[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-seq_register_finalizer "Copy/link to this API element")

@nocapture @C

```python
seq_register_finalizer(p: cobj, f: cobj)
```

function seq\_gc\_add\_roots[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-seq_gc_add_roots "Copy/link to this API element")

@nocapture @C

```python
seq_gc_add_roots(p: cobj, q: cobj)
```

function seq\_gc\_remove\_roots[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-seq_gc_remove_roots "Copy/link to this API element")

@nocapture @C

```python
seq_gc_remove_roots(p: cobj, q: cobj)
```

function seq\_gc\_clear\_roots[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-seq_gc_clear_roots "Copy/link to this API element")

@C

```python
seq_gc_clear_roots()
```

function seq\_gc\_exclude\_static\_roots[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-seq_gc_exclude_static_roots "Copy/link to this API element")

@nocapture @C

```python
seq_gc_exclude_static_roots(p: cobj, q: cobj)
```

function sizeof[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-sizeof "Copy/link to this API element")

```python
sizeof(T: type)
```

function atomic[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-atomic "Copy/link to this API element")

```python
atomic(T: type)
```

function alloc[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-alloc "Copy/link to this API element")

```python
alloc(sz: int)
```

function alloc\_atomic[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-alloc_atomic "Copy/link to this API element")

```python
alloc_atomic(sz: int)
```

function alloc\_uncollectable[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-alloc_uncollectable "Copy/link to this API element")

```python
alloc_uncollectable(sz: int)
```

function alloc\_atomic\_uncollectable[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-alloc_atomic_uncollectable "Copy/link to this API element")

```python
alloc_atomic_uncollectable(sz: int)
```

function realloc[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-realloc "Copy/link to this API element")

```python
realloc(p: cobj, newsz: int, oldsz: int)
```

function free[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-free "Copy/link to this API element")

```python
free(p: cobj)
```

function add\_roots[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-add_roots "Copy/link to this API element")

```python
add_roots(start: cobj, end: cobj)
```

function remove\_roots[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-remove_roots "Copy/link to this API element")

```python
remove_roots(start: cobj, end: cobj)
```

function clear\_roots[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-clear_roots "Copy/link to this API element")

```python
clear_roots()
```

function exclude\_static\_roots[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-exclude_static_roots "Copy/link to this API element")

```python
exclude_static_roots(start: cobj, end: cobj)
```

function register\_finalizer[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-register_finalizer "Copy/link to this API element")

```python
register_finalizer(p)
```

function construct\_ref[](https://docs.exaloop.io/libraries/api/modules/internal/gc/#function-construct_ref "Copy/link to this API element")

```python
construct_ref(args, T: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/gc/
