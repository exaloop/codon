Codon can be called from C/C++ code by compiling to a
shared library that can be linked to a C/C++ application.

Codon functions can be made externally visible by annotating
them with `@export` decorator:

``` python
@export
def foo(n: int):
    for i in range(n):
        print(i * i)
    return n * n
```

Note that only top-level, non-generic functions can be exported. Now we
can create a shared library containing `foo` (assuming source file
`foo.codon`):

``` bash
codon build --relocation-model=pic --lib -o libfoo.so foo.codon
```

Now we can call `foo` from a C program (if you're using C++, mark the
Codon function as `extern "C"`):

``` c
#include <stdint.h>
#include <stdio.h>

int64_t foo(int64_t);
// In C++, it would be:
// extern "C" int64_t foo(int64_t);

int main() {
  printf("%llu\n", foo(10));
}
```

Compile:

``` bash
gcc -o foo -L. -lfoo foo.c  # or g++ if using C++
```

Now running `./foo` will invoke `foo()` as defined in Codon, with an
argument of `10`.

Note that if the generated shared library is in a non-standard path, you
can either:

- Add the `rpath` to the `gcc` command: `-Wl,-rpath=/path/to/lib/dir`
- Add the library path to `LD_LIBRARY_PATH` (or `DYLD_LIBRARY_PATH` if
  using macOS): `export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/lib/dir`.

Type conversions between Codon and C/C++ are detailed [here](/integrations/cpp/cpp-from-codon#type-conversions).
