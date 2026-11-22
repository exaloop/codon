"""GPU module extraction and device lowering checks from the LLVM test suite."""

import re
from types import SimpleNamespace

import pytest

from ..ir import llvm


@pytest.mark.compiler
@pytest.mark.parametrize("debug", [True, False], ids=["debug", "release"])
def test_device_math_and_string_equality(llvm_library, codon_runtime, tmp_path, debug):
    libdevice = tmp_path / "libdevice.ll"
    libdevice.write_text("""
        define double @__nv_exp2(double %x) {
            %y = fadd double %x, 1.0
            ret double %y
        }
    """)
    ptx = tmp_path / "kernel.ptx"
    options = SimpleNamespace(
        jit=False, libdevice=str(libdevice), gpu_name="sm_70", gpu_features="+ptx70", ptx=ptx,
    )
    with llvm.Module(
        llvm.Library(llvm_library, codon_runtime), "device", "kernel.codon", options=options
    ) as module:
        module.parse_and_link("""
            declare ptr @__codon_ptx__()
            declare i32 @memcmp(ptr, ptr, i64)
            declare double @llvm.exp2.f64(double)
            declare {ptr, i64} @seq_locale_decimal_point()

            define ptr @ptx() {
                %result = call ptr @__codon_ptx__()
                ret ptr %result
            }

            define {} @kernel.name(ptr %out, ptr %a, ptr %b, i64 %len, double %x) "kernel" {
                %cmp = call i32 @memcmp(ptr %a, ptr %b, i64 %len)
                %same = icmp eq i32 %cmp, 0
                %locale = call {ptr, i64} @seq_locale_decimal_point()
                %decimal = extractvalue {ptr, i64} %locale, 0
                %byte = load i8, ptr %decimal
                %dot = uitofp i8 %byte to double
                %input = fadd double %dot, %x
                %y = call double @llvm.exp2.f64(double %input)
                %value = select i1 %same, double %y, double 0.0
                store double %value, ptr %out
                ret {} zeroinitializer
            }
        """)
        module.optimize(debug=debug)
        assert module.function("__codon_ptx__") is None
        assert module.global_variable(".ptx")
    text = ptx.read_text()
    assert ".visible .entry kernel_name(" in text
    assert re.search(r"\badd(?:\.[a-z]+)*\.f64\b", text)
    assert ") memcmp\n" not in text
    assert "seq_locale_decimal_point" not in text
    assert ".version 7.0" in text


@pytest.mark.compiler
def test_no_kernel_patches_ptx_to_null(llvm_library, codon_runtime):
    with llvm.Module(llvm.Library(llvm_library, codon_runtime), "host", "host.codon") as module:
        module.parse_and_link("""
            declare ptr @__codon_ptx__()
            declare i32 @personality()
            declare void @may_throw()
            @exception = global i8 0
            define ptr @ptx(i1 %first) personality ptr @personality {
            entry:
                br i1 %first, label %lookup, label %other
            lookup:
                %result = invoke ptr @__codon_ptx__() to label %normal unwind label %unwind
            normal:
                ret ptr %result
            other:
                invoke void @may_throw() to label %done unwind label %unwind
            done:
                ret ptr null
            unwind:
                %value = phi ptr [null, %lookup], [@exception, %other]
                %pad = landingpad {ptr, i32} cleanup
                ret ptr %value
            }
        """)
        module.optimize(debug=True)
        assert module.function("__codon_ptx__") is None
        assert "ret ptr null" in str(module)
        assert module.global_variable(".ptx") is None
