"""Exercise a relocated Windows SDK and linker arguments containing spaces."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def copy_file(source, destination):
    try:
        return os.link(source, destination)
    except OSError:
        return shutil.copy2(source, destination)


def run(arguments, env, cwd):
    result = subprocess.run(arguments, env=env, cwd=cwd,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            text=True)
    if result.returncode:
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        result.check_returncode()
    return result.stdout


sdk = Path(os.environ['CODON_DIR']).resolve()
with tempfile.TemporaryDirectory(prefix='codon sdk ', dir=Path(sys.argv[1]).resolve()) as directory:
    root = Path(directory)
    relocated = root / 'SDK with spaces'
    shutil.copytree(sdk, relocated, copy_function=copy_file)
    env = os.environ.copy()
    env.pop('CODON_PATH', None)
    env['CODON_DIR'] = str(relocated)
    # The CLI must load from its installed bin directory, without help from the
    # build tree or the original SDK's DLL directories.
    clang_dir = Path(shutil.which('clang')).parent
    system_root = Path(os.environ['SystemRoot'])
    env['PATH'] = os.pathsep.join(map(str, (clang_dir, system_root / 'System32', system_root)))
    runtime_env = env.copy()
    runtime_env['PATH'] = str(relocated / 'bin') + os.pathsep + env['PATH']
    compiler = relocated / 'bin' / 'codon.exe'
    plugin = root / 'installed SDK plugin'
    plugin.mkdir()
    plugin_library = plugin / 'rtti-pass.dll'
    fmt_library = next(p for p in (relocated / 'lib/codon/fmt.lib',
                                   relocated / 'lib/codon/fmtd.lib') if p.exists())
    run(['clang++', '-std=c++20', '-shared', '-fuse-ld=lld', '-fms-runtime-lib=dll',
         '-DNDEBUG', '-I' + str(relocated / 'include'),
         str(Path(__file__).with_name('sdk_plugin.cpp').resolve()),
         str(relocated / 'lib/codon/codonc.lib'), str(fmt_library),
         str(relocated / 'lib/codon/LLVMSupport.lib'),
         str(relocated / 'lib/codon/LLVMDemangle.lib'),
         '-lpsapi', '-lshell32', '-lole32', '-luuid', '-ladvapi32',
         '-lws2_32', '-lntdll', '-ldelayimp',
         '-o', str(plugin_library)], env, root)
    (plugin / 'plugin.toml').write_text(
        '[about]\nname = "SDK RTTI"\nsupported = ">=0.0.0"\n'
        '[library]\ncompiler = "rtti-pass"\nruntime = ""\n', encoding='utf-8')
    probe = root / 'plugin probe.codon'
    probe.write_text('from sys import argv\n@noinline\ndef sdk_probe(value: int):\n'
                     '    return value + 1\nprint(sdk_probe(len(argv)))\n', encoding='utf-8')
    assert run([str(compiler), 'run', '-release', '-plugin', str(plugin), str(probe)],
               env, root).strip() == '73'
    source = root / 'input with spaces.codon'
    source.write_text('print("SDK with spaces", flush=True)\n'
                      'with open("runtime stream.txt", "w") as stream:\n'
                      '    print("shared CRT", file=stream, flush=True)\n', encoding='utf-8')
    for mode in ('-debug', '-release'):
        output = root / f'my app {mode}.exe'
        run([str(compiler), 'build', mode, '-o', str(output), str(source)], env, root)
        assert run([str(output)], runtime_env, root).strip() == 'SDK with spaces'
        assert (root / 'runtime stream.txt').read_text().strip() == 'shared CRT'
        imports = run([shutil.which('llvm-readobj'), '--coff-imports', str(output)], env, root)
        assert 'Symbol: fflush' in imports, imports
        from windows_abi import check
        check(compiler, root, mode, env, runtime_env)
print('Windows SDK path-with-spaces checks passed')
