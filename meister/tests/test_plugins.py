"""Codon plugin manifest and runtime integration tests."""

from ctypes import CDLL
import sys

import pytest

from ..cache import Filesystem
from ..plugins import PluginError, PluginManager
from .support import compiler_environment, run_process


@pytest.fixture
def plugins(tmp_path):
    return PluginManager(Filesystem(str(tmp_path / "codon")), (0, 20, 7))


@pytest.mark.parametrize("version", ["", "*", ">=999.0.0 || >=0.0.0", "^0.20 || ~999.0"])
def test_supported_version(tmp_path, plugins, version):
    (tmp_path / "plugin.toml").write_text(
        f'[about]\nsupported = "{version}"\n[library]\nruntime = "missing-runtime"\n'
    )
    plugin = plugins.load(tmp_path)
    assert plugin.runtime.parent == tmp_path
    assert not plugins.runtime_libraries
    with pytest.raises(PluginError, match="could not load runtime library"):
        plugins.load_runtime_libraries()


@pytest.mark.parametrize(("manifest", "error"), [
    ('[about]\nsupported = ">=999.0.0"', "unsupported version"),
    ('[about]\nsupported = ">=0.0.0 @"', "invalid range"),
    ('[about]\nsupported = ">=0.0.0 ||"', "invalid range"),
    ('[about]\nsupported = " "', "invalid range"),
    ('[library]\nruntime = false', "library.runtime must be a string"),
    ('[library]\ncompiler = "compiler"', "require the Codon compiler ABI"),
])
def test_invalid_plugin(tmp_path, plugins, manifest, error):
    (tmp_path / "plugin.toml").write_text(manifest)
    with pytest.raises(PluginError, match=error):
        plugins.load(tmp_path)


def test_legacy_runtime_and_explicit_link_arguments(tmp_path, plugins):
    (tmp_path / "plugin.toml").write_text("""
        [library]
        cpp = "legacy"
        compiler = ""
        link = ["-lstandalone", "{root}/custom.a"]
    """)
    plugin = plugins.load(tmp_path)
    assert plugin.runtime.stem == "legacy"
    command = []
    plugins.add_link_libraries(command)
    assert command == ["-lstandalone", str(tmp_path / "custom.a")]


def test_installed_plugin(tmp_path):
    libdir = tmp_path / "lib" / "codon"
    root = libdir / "plugins" / "plugin"
    root.mkdir(parents=True)
    (root / "plugin.toml").write_text('[library]\ncodon = "stdlib"\n')
    plugins = PluginManager(Filesystem(str(libdir / "meister")), (0, 20, 7))
    assert plugins.load("plugin").root == root


@pytest.fixture
def runtime_plugin(codon_runtime, tmp_path):
    if sys.platform == "win32":
        pytest.skip("requires the POSIX runtime fixture")
    suffix = ".dylib" if sys.platform == "darwin" else ".so"
    filename = "libcodon_test_runtime_plugin" + suffix
    library = codon_runtime.parent / filename
    if not library.exists():
        pytest.skip("build the codon_test_runtime_plugin CMake target")
    root = tmp_path / "myplugin"
    module = root / "stdlib" / "myplugin"
    module.mkdir(parents=True)
    (root / filename).symlink_to(library)
    (module / "__init__.codon").write_text("from C import codon_test_runtime_value() -> int\n")
    manifest = root / "plugin.toml"
    manifest.write_text('[library]\ncodon = "stdlib"\nruntime = "libcodon_test_runtime_plugin"\n')
    return manifest, filename


@pytest.mark.compiler
@pytest.mark.parametrize("use_link_args", [False, True], ids=["default-link", "explicit-link"])
def test_runtime_plugin(llvm_library, codon_runtime, tmp_path, runtime_plugin, use_link_args):
    manifest, filename = runtime_plugin
    if use_link_args:
        manifest.write_text(manifest.read_text() + 'link = ["{root}/' + filename + '"]\n')
    source = tmp_path / "input.codon"
    source.write_text("from myplugin import codon_test_runtime_value\nprint(codon_test_runtime_value())\n")

    command = [sys.executable, "-m", "meister"]
    args = ["-release", "--llvm-library", str(llvm_library), "--runtime", str(codon_runtime)]
    env = compiler_environment(codon_runtime.parent)
    result = run_process([*command, "run", *args, str(source)], capture_output=True,
                         env=env, text=True, timeout=180)
    assert (result.returncode, result.stdout, result.stderr) == (0, "42\n", "")
    output = tmp_path / "program"
    result = run_process([*command, "build", *args, "-o", str(output), str(source)],
                         capture_output=True, env=env, text=True, timeout=180)
    assert result.returncode == 0, result.stderr
    result = run_process([str(output)], capture_output=True, env=env, text=True, timeout=30)
    assert (result.returncode, result.stdout, result.stderr) == (0, "42\n", "")


@pytest.mark.compiler
def test_runtime_plugin_loaded_in_later_cell(llvm_library, codon_runtime, tmp_path, runtime_plugin):
    if not hasattr(CDLL(str(codon_runtime)), "seq_jit_terminate"):
        pytest.skip("requires the incremental JIT runtime C API")
    result = run_process(
        [sys.executable, "-m", "meister", "jit", "-release", "--llvm-library",
         str(llvm_library), "--runtime", str(codon_runtime)],
        input="print(7)\n#%%\nfrom myplugin import codon_test_runtime_value\n"
              "print(codon_test_runtime_value())\n#%%\nprint(codon_test_runtime_value())\n",
        cwd=tmp_path, capture_output=True, env=compiler_environment(codon_runtime.parent),
        text=True, timeout=240,
    )
    assert result.returncode == 0, (result.stdout, result.stderr)
    assert not result.stderr
    assert result.stdout.splitlines()[1:] == ["7", "[done]", "42", "[done]", "42", "[done]"]
