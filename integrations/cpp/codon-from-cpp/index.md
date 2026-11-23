# Call Codon from C/C++

Codon can be called from C/C++ code by compiling to a shared library that can be linked to a C/C++ application.

Codon functions can be made externally visible by annotating them with `@export` decorator:

```python
@export
def foo(n: int):
    for i in range(n):
        print(i * i)
    return n * n
```

Note that only top-level, non-generic functions can be exported. Now we can create a shared library containing `foo` (assuming source file `foo.codon`):

```bash
codon build -release --relocation-model=pic --lib -o libfoo.so foo.codon
```

Now we can call `foo` from a C program (if you're using C++, mark the Codon function as `extern "C"`):

```c
#include <stdint.h>
#include <stdio.h>

int64_t foo(int64_t);
// In C++, it would be:
// extern "C" int64_t foo(int64_t);

int main() {
  printf("%lld\n", (long long)foo(10));
  return 0;
}
```

Compile:

```bash
cc foo.c -L. -lfoo -Wl,-rpath,"$PWD" -o foo
```

Running `./foo` invokes the Codon function with an argument of `10`, printing the squares from `0` through `81` and then its return value, `100`. A C++ caller can use `c++` with the `extern "C"` declaration shown above.

The link command places the caller before `-lfoo`, which matters on linkers that resolve libraries in command-line order. Its rpath points to the current directory so the example can locate `libfoo.so` when run. A deployed application should instead use its intended library location and include the Codon runtime dependencies required by the generated library. Library search paths can also be configured with `LD_LIBRARY_PATH` on Linux or `DYLD_LIBRARY_PATH` on macOS.

Type conversions between Codon and C/C++ are detailed [here](https://docs.exaloop.io/integrations/cpp/cpp-from-codon/#type-conversions).

Source: https://docs.exaloop.io/integrations/cpp/codon-from-cpp/
