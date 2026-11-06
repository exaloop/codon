"""Compare Codon's direct, indirect and exported POD ABI against Clang C."""
from pathlib import Path
import json
import subprocess


def check(compiler, root, mode, env, runtime_env):
    cases = []
    for count in (*range(1, 9), 16):
        fields = [f'data[{i}]' for i in range(count)]
        cases.append((f'b{count}', f'uint8_t data[{count}];',
                      ['u8'] * count, fields, [11 + 7 * i for i in range(count)], False))
    cases += [
        ('i8', 'int32_t a, b;', ['i32', 'i32'], ['a', 'b'], [12, 34], False),
        ('f4', 'float a;', ['float32'], ['a'], [1.25], True),
        ('f8', 'float a, b;', ['float32', 'float32'], ['a', 'b'], [1.25, -2.5], True),
        ('mixed8', 'int32_t a; float b;', ['i32', 'float32'], ['a', 'b'], [12, 3.5], True),
    ]
    declarations = ['#include <stdint.h>', '#include <windows.h>']
    client = ['int main(int argc, char **argv) {',
              'HMODULE module = LoadLibraryA(argv[1]); if (!module) return 1;']
    program = ['from internal.dlopen import dlsym_c']
    exported = []
    library = root / f'abi fixture {mode}.dll'
    import_library = library.with_suffix('.lib')
    for name, definition, types, fields, values, floating in cases:
        ctype = f'S_{name}'
        ty = 'Tuple[' + ', '.join(types) + ']'
        def tup(items):
            return '(' + ', '.join(f'{t}({v!r})' for t, v in zip(types, items)) + ',)'
        declarations.append(f'typedef struct {{ {definition} }} {ctype};')
        result_type = 'double' if floating else 'int64_t'
        terms = [f'(({result_type})x.{field})' for field in fields]
        if name == 'i8':
            terms = ['((int64_t)x.a)', '1000 * ((int64_t)x.b)']
        declarations.append(f'__declspec(dllexport) {result_type} score_{name}({ctype} x) {{ return ' + ' + '.join(terms) + '; }')
        declarations.append(f'__declspec(dllexport) {result_type} score_callback_{name}({result_type} (*fn)({ctype}), {ctype} x) {{ return fn(x); }}')
        declarations.append(f'__declspec(dllexport) {ctype} bump_{name}({ctype} x) {{ ' + ''.join(f'x.{field} += 1;' for field in fields) + ' return x; }')
        declarations.append(f'__declspec(dllexport) {ctype} callback_{name}({ctype} (*fn)({ctype}), {ctype} x) {{ return fn(x); }}')
        declarations.append(f'typedef {ctype} (*Fn_{name})({ctype});')
        declarations.append(f'__declspec(dllexport) Fn_{name} get_bump_{name}(void) {{ return bump_{name}; }}')
        program += [f'from C import score_{name}({ty}) -> {"float" if floating else "int"}',
                    f'from C import bump_{name}({ty}) -> {ty}',
                    f'value_{name} = {tup(values)}',
                    f'assert score_{name}(value_{name}) == {34012 if name == "i8" else sum(values)!r}',
                    f'assert bump_{name}(value_{name}) == {tup([v + 1 for v in values])}',
                    f'indirect_{name} = dlsym_c({json.dumps(library.as_posix())}, "bump_{name}", Function[[{ty}], {ty}])',
                    f'assert indirect_{name}(value_{name}) == {tup([v + 1 for v in values])}',
                    '@export', f'def echo_{name}(value: {ty}) -> {ty}:', '    return value']
        exported.append(f'echo_{name}')
        score_type = 'float' if floating else 'int'
        expected_score = 34012 if name == 'i8' else sum(values)
        score_terms = [f'{score_type}(x[{i}])' for i in range(len(types))]
        if name == 'i8':
            score_terms[1] = '1000 * ' + score_terms[1]
        program += [
            f'from C import score_callback_{name}(Function[[{ty}], {score_type}], {ty}) -> {score_type}',
            '@noinline',
            f'def apply_score_{name}(fn: Function[[{ty}], {score_type}], x: {ty}) -> {score_type}:',
            '    return fn(x)',
            f'native_score_{name}: Function[[{ty}], {score_type}] = score_{name}',
            f'assert apply_score_{name}(native_score_{name}, value_{name}) == {expected_score!r}',
            '@export', f'def exported_score_{name}(x: {ty}) -> {score_type}:',
            '    return ' + ' + '.join(score_terms),
            f'assert score_callback_{name}(exported_score_{name}, value_{name}) == {expected_score!r}',
            f'from C import callback_{name}(Function[[{ty}], {ty}], {ty}) -> {ty}',
            f'from C import get_bump_{name}() -> Function[[{ty}], {ty}]',
            '@noinline',
            f'def apply_{name}(fn: Function[[{ty}], {ty}], x: {ty}) -> {ty}:',
            '    return fn(x)',
            '@noinline', f'def local_echo_{name}(x: {ty}) -> {ty}:', '    return x',
            f'native_{name}: Function[[{ty}], {ty}] = bump_{name}',
            f'assert apply_{name}(native_{name}, value_{name}) == {tup([v + 1 for v in values])}',
            f'assert apply_{name}(get_bump_{name}(), value_{name}) == {tup([v + 1 for v in values])}',
            f'assert callback_{name}(native_{name}, value_{name}) == {tup([v + 1 for v in values])}',
            f'assert apply_{name}(echo_{name}, value_{name}) == value_{name}',
            f'assert apply_{name}(local_echo_{name}, value_{name}) == value_{name}',
            f'assert callback_{name}(echo_{name}, value_{name}) == value_{name}',
            f'assert callback_{name}(local_echo_{name}, value_{name}) == value_{name}',
            '@noinline',
            f'def choose_{name}(exported: bool) -> Function[[{ty}], {ty}]:',
            f'    public: Function[[{ty}], {ty}] = echo_{name}',
            f'    local: Function[[{ty}], {ty}] = local_echo_{name}',
            '    return public if exported else local',
            'for exported_callback in (False, True):',
            f'    selected_{name} = choose_{name}(exported_callback)',
            f'    assert apply_{name}(selected_{name}, value_{name}) == value_{name}',
            f'    assert callback_{name}(selected_{name}, value_{name}) == value_{name}',
            '@export', f'def try_bump_{name}(x: {ty}) -> {ty}:',
            '    try:', f'        return bump_{name}(x)',
            '    except ValueError:', '        return x',
            f'assert try_bump_{name}(value_{name}) == {tup([v + 1 for v in values])}',
        ]
        client += ['{', f'{ctype} value = {{0}};',
                   *(f'value.{field} = {v!r};' for field, v in zip(fields, values)),
                   f'{ctype} (*call)({ctype}) = ({ctype} (*)({ctype}))GetProcAddress(module, "echo_{name}");',
                   f'if (!call) return 2; {ctype} result = call(value);',
                   *(f'if (result.{field} != value.{field}) return 3;' for field in fields),
                   f'{result_type} (*score)({ctype}) = ({result_type} (*)({ctype}))GetProcAddress(module, "exported_score_{name}");',
                   f'if (!score || score(value) != {expected_score!r}) return 4;', '}']
    client += ['return 0;', '}']
    program += ['from C import seq_locale_decimal_point() -> str',
                '@export', 'def try_decimal_point() -> int:',
                '    try:', '        return len(seq_locale_decimal_point())',
                '    except ValueError:', '        return -1',
                'assert try_decimal_point() == len(seq_locale_decimal_point())',
                'expected_point = seq_locale_decimal_point()',
                'for iteration in range(300000):',
                '    assert seq_locale_decimal_point() == expected_point']
    fixture_source = root / 'C ABI fixture.c'
    fixture_source.write_text('\n'.join(declarations), encoding='utf-8')
    subprocess.run(['clang', '-shared', '-fms-runtime-lib=dll', str(fixture_source),
                    '-Wl,/nodefaultlib:libcmt',
                    f'-Wl,/IMPLIB:{import_library}', '-o', str(library)],
                   env=env, cwd=root, check=True)
    source = root / 'C ABI checks.codon'
    source.write_text('\n'.join(program), encoding='utf-8')
    executable = root / f'ABI checks {mode}.exe'
    flags = '-linker-flags="' + import_library.as_posix() + '" -Wl,/stack:1048576'
    subprocess.run([str(compiler), 'build', mode, '-o', str(executable), flags, str(source)],
                   env=env, cwd=root, check=True)
    subprocess.run([str(executable)], env=runtime_env, cwd=root, check=True)
    codon_library = root / f'Codon exports {mode}.dll'
    subprocess.run([str(compiler), 'build', mode, '-o', str(codon_library),
                    flags, str(source)], env=env, cwd=root, check=True)
    client_source = root / 'C ABI consumer.c'
    client_source.write_text('\n'.join(declarations[:2] +
                           [f'typedef struct {{ {definition} }} S_{name};'
                            for name, definition, *_ in cases] + client), encoding='utf-8')
    client_exe = root / 'C ABI consumer.exe'
    subprocess.run(['clang', '-fms-runtime-lib=dll', '-Wl,/nodefaultlib:libcmt',
                    str(client_source), '-o', str(client_exe)],
                   env=env, cwd=root, check=True)
    subprocess.run([str(client_exe), str(codon_library)], env=runtime_env, cwd=root, check=True)
    print(f'Win64 POD ABI checks passed ({mode})', flush=True)
