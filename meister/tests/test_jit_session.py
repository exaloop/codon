"""Exercise a persistent JIT and late plugin loading through the CLI."""

from ctypes import CDLL
import sys

import pytest

from .support import compiler_environment, run_process


@pytest.mark.compiler
@pytest.mark.parametrize("mode", ["debug", "release"])
def test_jit_session_preserves_layout_and_recovers(llvm_library, codon_runtime, tmp_path, mode):
    if sys.platform == "win32" or not hasattr(CDLL(str(codon_runtime)), "seq_jit_terminate"):
        pytest.skip("requires the incremental JIT runtime C API")
    source = tmp_path / "cells.codon"
    source.write_text("""\
class LayoutBase(object):
    balance: int
    def method(self, *values) -> int:
        return len(values)

class LayoutChild(LayoutBase):
    def method(self, *values) -> int:
        return 100 + len(values)

@noinline
def update_old(value: LayoutBase):
    value.balance += 1
    return value.balance

@noinline
def dispatch_old(value: LayoutBase):
    return value.method(1)

old: LayoutBase = LayoutChild(40)
assert update_old(old) == 41
assert dispatch_old(old) == 101
old_info = TypeInfo.cache(LayoutChild)
old_offsets = old_info.offsets
old_methods = old_info.methods
print('first')
#%%
@noinline
def dispatch_new(value: LayoutBase):
    return value.method(1, 'two')

assert dispatch_new(old) == 102
assert dispatch_old(old) == 101
assert old_info.offsets == old_offsets and old_info.methods != old_methods
assert update_old(old) == 42
print('second')
#%%
class LayoutBad(LayoutBase):
    def method(self, *values) -> str:
        return 'incompatible'

bad: LayoutBase = LayoutBad(0)
dispatch_old(bad)
#%%
raise ValueError('cell failed')
#%%
unknown_symbol()
#%%
assert update_old(old) == 43
dispatch_old(old)
""")
    result = run_process(
        [sys.executable, "-m", "meister", "jit", "-" + mode, "--llvm-library", str(llvm_library),
         "--runtime", str(codon_runtime), str(source)],
        capture_output=True, text=True, env=compiler_environment(codon_runtime.parent), timeout=240,
    )
    assert result.returncode == 0, (result.stdout, result.stderr)
    assert "first\n[done]\nsecond\n[done]\n" in result.stdout
    assert "cell failed" in result.stdout
    assert result.stdout.endswith("101[done]\n")
    assert result.stdout.count("[done]\n") == 6
    assert "returns str, expected int" in result.stderr
    assert "unknown_symbol" in result.stderr
