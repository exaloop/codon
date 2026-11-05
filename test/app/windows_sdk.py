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
    return subprocess.run(arguments, env=env, cwd=cwd, check=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          text=True).stdout


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
