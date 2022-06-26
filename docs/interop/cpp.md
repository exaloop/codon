Calling C/C++ from Codon is quite easy with `from C import`, but Codon
can also be called from C/C++ code. To make a Codon function externally
visible, simply annotate it with `@export`:

``` python
@export
def foo(n: int):
    for i in range(n):
        print(i * i)
    return n * n
```

Note that only top-level, non-generic functions can be exported. Now we
can create a shared library containing `foo` (assuming source file
*foo.codon*):

``` bash
codon build -o libfoo.so foo.codon
```

Now we can call `foo` from a C program:

``` c
#include <stdint.h>
#include <stdio.h>

int64_t foo(int64_t);

int main() {
  printf("%llu\n", foo(10));
}
```

Compile:

``` bash
gcc -o foo -L. -lfoo foo.c
```

Now running `./foo` should invoke `foo()` as defined in Codon, with an
argument of `10`.

# Converting types

The following table shows the conversions between Codon and C/C++ types:

  | Codon     | C/C++                                |
  |-----------|--------------------------------------|
  | `int`     | `long` or `int64_t`                  |
  | `float`   | `double`                             |
  | `bool`    | `bool`                               |
  | `byte`    | `char` or `int8_t`                   |
  | `str`     | `{int64_t, char*}` (length and data) |
  | `class`   | Pointer to corresponding tuple       |
  | `@tuple`  | Struct of fields                     |
