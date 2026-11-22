"""LLVM pass regressions from test/cir/llvm/optimize.cpp."""

from contextlib import contextmanager
import sys

import pytest

from ..cache import FrontendOptions
from ..ir.codegen import emit_module, prepare_compiler
from ..ir.llvm import I, U, Library, Module
from ..ir.llvm_cfg import FunctionCFG
from .cases import TestCase as SourceCase
from .runner import compile_cir
from .support import compiler_environment, run_process


@pytest.fixture
def optimize_ir(llvm_library, codon_runtime):
    library = Library(llvm_library, codon_runtime)

    @contextmanager
    def optimize(source, *, auto_free=False):
        with Module(library, "allocation_hoister_test", "allocation_hoister_test.codon") as module:
            module.parse_and_link(source)
            module.verify()
            module.optimize(debug=False, native=False, auto_free=auto_free)
            yield module
    return optimize


@pytest.mark.parametrize("allocator", (
    "seq_alloc", "seq_alloc_atomic", "seq_alloc_uncollectable", "seq_alloc_atomic_uncollectable",
))
def test_aligns_demoted_allocations_for_garbage_collection(optimize_ir, allocator):
    with optimize_ir(f"""
declare noalias ptr @{allocator}(i64)
declare i64 @collect()
declare void @llvm.memcpy.p0.p0.i64(ptr nocapture writeonly, ptr nocapture readonly, i64, i1 immarg)

define i8 @test(ptr %source) {{
entry:
  %allocation = call ptr @{allocator}(i64 17)
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %allocation, ptr align 1 %source, i64 17, i1 false)
  %index = call i64 @collect()
  %selected = getelementptr i8, ptr %allocation, i64 %index
  %result = load i8, ptr %selected, align 1
  ret i8 %result
}}
""") as module:
        instructions = [
            inst for block in module.function("test").blocks() for inst in block.instructions()
        ]
        allocas = [inst for inst in instructions if inst.is_a("AllocaInst")]
        assert len(allocas) == 1
        assert allocas[0]._call("GetAlignment", U, ()) == (
            1 if "atomic" in allocator else module.data_layout.abi_alignment(module.pointer_type())
        )
        assert not any(i.is_call and i.called_function and i.called_function.name == allocator
                       for i in instructions)


def make_allocation_loop(declarations, use):
    return """
declare noalias ptr @seq_alloc_atomic(i64)
""" + declarations + """
define i64 @test(i64 %count) {
entry:
  br label %header
header:
  %index = phi i64 [ 0, %entry ], [ %next, %body ]
  %total = phi i64 [ 0, %entry ], [ %updated, %body ]
  %done = icmp eq i64 %index, %count
  br i1 %done, label %exit, label %body
body:
  %allocation = call ptr @seq_alloc_atomic(i64 65536)
""" + use + """
  %extended = zext i8 %value to i64
  %updated = add i64 %total, %extended
  %next = add i64 %index, 1
  br label %header
exit:
  ret i64 %total
}
"""


@pytest.mark.parametrize(("attributes", "function_attributes", "hoisted"), [
    ("ptr nocapture", "", False),
    ("ptr", "nofree", False),
    ("ptr nocapture", "nofree", True),
], ids=("without-nofree", "without-nocapture", "metadata-only"))
def test_hoists_only_safe_allocations(optimize_ir, attributes, function_attributes, hoisted):
    ir = make_allocation_loop(
        f"declare i8 @consume({attributes}) {function_attributes}\n",
        "  %value = call i8 @consume(ptr %allocation)\n",
    )
    with optimize_ir(ir) as module:
        cfg = FunctionCFG(module.function("test"))
        allocations = [
            (block, inst) for block, handle in cfg.blocks.items() for inst in handle.instructions()
            if inst.is_call and inst.called_function and inst.called_function.name == "seq_alloc_atomic"
        ]
        assert len(allocations) == 1
        block, alloc = allocations[0]
        assert cfg.loop_for(block)
        assert any(user.is_a("PHINode") for user in alloc.users()) == hoisted


@pytest.mark.parametrize("same_initial", [False, True])
@pytest.mark.parametrize("null_edge", [False, True])
def test_merges_equivalent_guarded_pointer_states(optimize_ir, same_initial, null_edge):
    ir = """
declare ptr @advance(ptr)
define i1 @test(ptr %initial, ptr %other, i64 %count) {
entry:
  br label %header
header:
  %left = phi ptr [ %initial, %entry ], [ %left.next, %latch ]
  %right = phi ptr [ INITIAL, %entry ], [ %right.next, %latch ]
  %index = phi i64 [ 0, %entry ], [ %next, %latch ]
  %finished = icmp eq i64 %index, %count
  br i1 %finished, label %exit, label %pull
pull:
  %done = icmp PREDICATE ptr %right, null
  br i1 %done, label %latch, label %resume
resume:
  %value = call ptr @advance(ptr %left)
  br label %latch
latch:
  %left.next = phi ptr [ %left, %pull ], [ %value, %resume ]
  %right.next = phi ptr [ null, %pull ], [ %value, %resume ]
  %next = add i64 %index, 1
  br label %header
exit:
  %equal = icmp eq ptr %left, %right
  ret i1 %equal
}
"""
    ir = ir.replace("INITIAL", "%initial" if same_initial else "%other")
    ir = ir.replace("PREDICATE", "eq" if null_edge else "ne")
    with optimize_ir(ir) as module:
        returns = [b.terminator.operand(0) for b in module.function("test").blocks()
                   if b.terminator.is_a("ReturnInst")]
        # LLVMGetSExtValue returns -1 for a true i1.
        assert returns
        assert all(r.signed_int() == -1 for r in returns) == (same_initial and null_edge)


@pytest.mark.parametrize("callee_noinline", [False, True])
@pytest.mark.parametrize("call_noinline", [False, True])
def test_respects_resume_noinline_attributes(optimize_ir, callee_noinline, call_noinline):
    with optimize_ir(f"""
define internal void @values.resume(ptr nocapture %frame) "codon.generator" {'noinline' if callee_noinline else ''} {{
entry:
  %value = load i64, ptr %frame
  %updated = add i64 %value, 1
  store i64 %updated, ptr %frame
  ret void
}}
define i64 @test(i64 %initial) {{
entry:
  %frame = alloca i64
  store i64 %initial, ptr %frame
  call void @values.resume(ptr %frame) {'noinline' if call_noinline else ''}
  %result = load i64, ptr %frame
  ret i64 %result
}}
""") as module:
        has_resume = any(
            inst.is_call and inst.called_function and inst.called_function.name.startswith("values.resume")
            for block in module.function("test").blocks() for inst in block.instructions()
        )
        assert has_resume == (callee_noinline or call_noinline)


def call_sites(cfg, name):
    for key, block in cfg.blocks.items():
        for inst in block.instructions():
            if inst.is_call and inst.called_function and inst.called_function.name == name:
                yield key, inst


@pytest.mark.parametrize("owner", ["allocation", "selected", "escaped"])
def test_hoists_and_releases_freed_loop_allocation(optimize_ir, owner):
    declarations = """
@escaped = global ptr null
declare void @seq_free(ptr) nounwind
declare i8 @read(ptr nocapture) nofree memory(read)
declare ptr @other()
declare i1 @choose()
"""
    use = "  store i8 42, ptr %allocation\n  %value = call i8 @read(ptr %allocation)\n"
    if owner == "selected":
        use += """
  %other = call ptr @other()
  %choose = call i1 @choose()
  %owner = select i1 %choose, ptr %allocation, ptr %other
  call void @seq_free(ptr %owner)
"""
    else:
        if owner == "escaped":
            use += "  store ptr %allocation, ptr @escaped\n"
        use += "  call void @seq_free(ptr %allocation)\n"
    with optimize_ir(make_allocation_loop(declarations, use)) as module:
        cfg = FunctionCFG(module.function("test"))
        frees = list(call_sites(cfg, "seq_free"))
        assert len(frees) == 1
        assert bool(cfg.loop_for(frees[0][0])) == (owner != "allocation")


@pytest.mark.parametrize("observe_pointer", [False, True])
def test_hoists_metadata_fields_through_aggregate_helpers(optimize_ir, observe_pointer):
    observed = (
        "  %pointer = extractvalue { ptr, i64 } %nested, 0\n"
        "  call void @escape(ptr %pointer)\n"
    ) if observe_pointer else ""
    declarations = """
declare i8 @observe_size(i64)
declare i8 @read(ptr nocapture) nofree
declare void @escape(ptr)

define i8 @metadata({ { ptr, i64 }, i64 } %descriptor) noinline optnone {
  %nested = extractvalue { { ptr, i64 }, i64 } %descriptor, 0
""" + observed + """
  %size = extractvalue { ptr, i64 } %nested, 1
  %result = call i8 @observe_size(i64 %size)
  ret i8 %result
}
"""
    use = """
  %descriptor = insertvalue { { ptr, i64 }, i64 } zeroinitializer, ptr %allocation, 0, 0
  %sized = insertvalue { { ptr, i64 }, i64 } %descriptor, i64 %index, 0, 1
  %metadata = call i8 @metadata({ { ptr, i64 }, i64 } %sized)
  %byte = call i8 @read(ptr %allocation)
  %value = add i8 %metadata, %byte
"""
    with optimize_ir(make_allocation_loop(declarations, use)) as module:
        cfg = FunctionCFG(module.function("test"))
        _, alloc = next(call_sites(cfg, "seq_alloc_atomic"))
        assert any(user.is_a("PHINode") for user in alloc.users()) == (not observe_pointer)


def test_preserves_user_unroll_metadata(optimize_ir):
    with optimize_ir("""
define i64 @user_unroll_guard(ptr %data, i64 %count) {
entry:
  br label %loop
loop:
  %index = phi i64 [ 0, %entry ], [ %next, %loop ]
  %total = phi i64 [ 0, %entry ], [ %updated, %loop ]
  %address = getelementptr i64, ptr %data, i64 %index
  %value = load volatile i64, ptr %address
  %updated = add i64 %total, %value
  %next = add i64 %index, 1
  %again = icmp ult i64 %next, %count
  br i1 %again, label %loop, label %exit, !llvm.loop !0
exit:
  ret i64 %updated
}
!0 = distinct !{!0, !1, !2}
!1 = !{!"llvm.loop.unroll.disable"}
!2 = !{!"codon.test.preserve"}
""") as module:
        ir = str(module)
        assert '!"llvm.loop.unroll.disable"' in ir
        assert '!"codon.test.preserve"' in ir
        assert '!"codon.coro.unroll"' not in ir


def test_auto_free_after_last_use(optimize_ir):
    with optimize_ir("""
declare ptr @seq_alloc_atomic(i64)
declare i8 @read(ptr nocapture) nofree

define i8 @test(i64 %size) {
entry:
  %allocation = call ptr @seq_alloc_atomic(i64 %size)
  %value = call i8 @read(ptr %allocation)
  ret i8 %value
}
""", auto_free=True) as module:
        cfg = FunctionCFG(module.function("test"))
        assert len(list(call_sites(cfg, "seq_free"))) == 1


@pytest.mark.compiler
def test_executes_hoisted_freed_loop_allocations(llvm_library, codon_runtime, tmp_path, pytestconfig):
    # LLVMOptimizationTest.ExecutesHoistedFreedLoopAllocations.
    source = tmp_path / "allocation_free_test.codon"
    source.write_text("""
from internal.gc import free

@C
def GC_get_total_bytes() -> int:
  pass

@noinline
def touch(buffer: Ptr[byte], size: int, value: int):
  for offset in range(size):
    buffer[offset] = byte(value + offset)
  return int(buffer[0]) + int(buffer[size - 1])

@noinline
def exercise(limit: int, count: int, stop: int, skip: bool):
  total = 0
  for outer in range(limit):
    size = (outer + 1) * 4096
    for inner in range(count):
      if skip or inner % 2 == 0:
        continue
      buffer = Ptr[byte](size)
      total += touch(buffer, size, outer + inner)
      free(buffer.as_byte())
      if inner == stop:
        return total
  return total

def expected(limit: int, count: int, stop: int, skip: bool):
  total = 0
  for outer in range(limit):
    for inner in range(count):
      if skip or inner % 2 == 0:
        continue
      total += ((outer + inner) & 255) + ((outer + inner - 1) & 255)
      if inner == stop:
        return total
  return total

for count in (0, 1, 8):
  for stop in (-1, 3):
    for skip in (False, True):
      assert exercise(8, count, stop, skip) == expected(8, count, stop, skip)

before = GC_get_total_bytes()
result = exercise(8, 128, -1, False)
allocated = GC_get_total_bytes() - before
assert result == expected(8, 128, -1, False)
assert 0 < allocated < 1024 * 1024
""", encoding="utf-8")
    result = run_process(
        [sys.executable, "-m", "meister", "run", "-release", "--library", str(llvm_library),
         "--runtime", str(codon_runtime), str(source)],
        env=compiler_environment(codon_runtime.parent), capture_output=True,
        timeout=pytestconfig.getoption("--codon-timeout"),
    )
    assert result.returncode == 0, result.stderr.decode(errors="replace")



@contextmanager
def optimize_source(llvm_library, codon_runtime, source, path, *, disabled=()):
    options = FrontendOptions(debug=False, disabled=disabled)
    cache, defines = prepare_compiler(codon_runtime, options)
    compile_cir(cache, defines, SourceCase(path, code=source))
    library = Library(llvm_library, codon_runtime)
    with emit_module(cache, library, str(path), defines, standalone=True) as module:
        module.optimize(debug=False)
        yield module


def verify_elided_iteration(function):
    assert function
    for block in function.blocks():
        for inst in block.instructions():
            assert not inst.is_a("AllocaInst")
            if inst.is_call or inst.is_a("InvokeInst"):
                callee = inst.called_function
                assert callee and callee._call("GetIntrinsicID", U, ())
                assert not callee.name.startswith("llvm.coro.")


@pytest.mark.compiler
def test_elides_repeated_generator_pulls(llvm_library, codon_runtime, tmp_path):
    source = """
def values(data: Ptr[int], count: int):
  for index in range(count):
    yield data[index]

@export
def return_generator(data: Ptr[int], count: int):
  return values(data, count)

@export
def store_generator(data: Ptr[int], count: int, output: Ptr[Generator[int]]):
  output[0] = values(data, count)
"""
    for pulls in (16, 32, 64):
        source += f"""
@export
def pull_{pulls}(data: Ptr[int], count: int) -> int:
  items = values(data, count)
  total = 0
"""
        source += "  total += next(items, -1)\n" * pulls
        source += "  return total\n"
    with optimize_source(llvm_library, codon_runtime, source, tmp_path / "allocation_phi_test.codon") as module:
        for pulls in (16, 32, 64):
            verify_elided_iteration(module.function(f"pull_{pulls}"))
        for name in ("return_generator", "store_generator"):
            assert list(call_sites(FunctionCFG(module.function(name)), "seq_alloc"))



@pytest.mark.compiler
def test_elides_safe_generator_and_zip_iteration(llvm_library, codon_runtime, tmp_path):
    source = """
def generator_values(data: Ptr[int], count: int):
  index = 0
  while index < count:
    yield data[index]
    index += 1

@export
def consume_generator(data: Ptr[int], count: int) -> int:
  total = 0
  for value in generator_values(data, count):
    total += value
  return total

@export
def consume_zip(data: Ptr[int], count: int) -> int:
  total = 0
  for left, right in zip(generator_values(data, count), generator_values(data, count)):
    total += left + right
  return total

@export
def consume_aliased_zip(data: Ptr[int], count: int) -> int:
  total = 0
  items = generator_values(data, count)
  for left, right in zip(items, items):
    total += left + right
  return total

def range_values(data: Ptr[int], count: int):
  for index in range(count):
    yield data[index]

def delegated_values(data: Ptr[int], count: int):
  yield from range_values(data, count)
  yield from range_values(data + 1, count)

@export
def consume_delegated(data: Ptr[int], count: int) -> int:
  total = 0
  for value in delegated_values(data, count):
    total += value
  return total

@export
def consume_nested_zip(data: Ptr[int], count: int) -> int:
  total = 0
  inner = zip(range_values(data, count), range_values(data + 1, count))
  for pair, third in zip(inner, range_values(data + 2, count)):
    total += pair[0] * pair[1] + third
  return total

@export
def escaping_generator(data: Ptr[int], count: int):
  return generator_values(data, count)
"""
    with optimize_source(llvm_library, codon_runtime, source, tmp_path / "allocation_phi_test.codon") as module:
        for name in ("consume_generator", "consume_zip", "consume_aliased_zip",
                     "consume_delegated", "consume_nested_zip"):
            verify_elided_iteration(module.function(name))
        for name in ("consume_delegated", "consume_nested_zip"):
            assert any(
                inst.is_a("BinaryOperator") and inst.type._call("GetTypeKind", I, ()) in (13, 17)
                for block in module.function(name).blocks() for inst in block.instructions()
            )
        assert list(call_sites(FunctionCFG(module.function("escaping_generator")), "seq_alloc"))



@pytest.mark.compiler
@pytest.mark.parametrize("disable_fusion", [False, True])
def test_folds_filtered_generators_without_premature_unrolling(
    llvm_library, codon_runtime, tmp_path, disable_fusion,
):
    source = """
@export
def filtered_start() -> int:
  return sum(filter(lambda value: value % 2 == 0,
                    (index for index in range(25))), 7)

@export
def filtered_reject() -> int:
  return sum(filter(lambda value: value < 0,
                    (index for index in range(25))))

@export
def unfiltered() -> int:
  return sum(index for index in range(25))

@export
def consumed() -> int:
  total = 0
  for value in filter(lambda value: value % 2 == 0,
                      (index for index in range(25))):
    total += value
  return total

@export
def ordinary(data: Ptr[int]) -> int:
  total = 0
  for index in range(4):
    total += data[index]
  return total

def yielding(data: Ptr[int]):
  for value in range(25):
    total = 0
    for index in range(4):
      total += data[index]
    yield total + value

@export
def escaping(data: Ptr[int]):
  return yielding(data)
"""
    counts = (0, 1, 2, 24, 25, 26, 64)
    for count in counts:
        source += f"""
@export
def filtered_{count}() -> int:
  return sum(filter(lambda value: value % 2 == 0, (index for index in range({count}))))
"""
    disabled = ("core-pythonic-generator-loop-fusion",) if disable_fusion else ()
    with optimize_source(
        llvm_library, codon_runtime, source, tmp_path / "allocation_phi_test.codon", disabled=disabled,
    ) as module:
        assert "codon.coro.unroll" not in str(module)
        expected = {f"filtered_{count}": sum(range(0, count, 2)) for count in counts}
        expected.update(filtered_start=163, filtered_reject=0, unfiltered=300)
        if not disable_fusion:
            expected["consumed"] = 156
        for name, total in expected.items():
            blocks = list(module.function(name).blocks())
            assert len(blocks) == 1, name
            assert len(list(blocks[0].instructions())) == 1, name
            assert blocks[0].terminator.operand(0).signed_int() == total, name
        assert not FunctionCFG(module.function("ordinary")).loops
        resumes = [fn for fn in module.functions() if "yielding" in fn.name and fn.name.endswith(".resume")]
        assert resumes
        assert all(not FunctionCFG(fn).loops for fn in resumes)



def test_hoists_freed_allocation_with_conditional_use_and_early_exit(optimize_ir):
    with optimize_ir("""
declare noalias ptr @seq_alloc_atomic(i64)
declare void @seq_free(ptr) nounwind
declare i8 @read(ptr nocapture) nofree memory(read)
declare void @early_exit()

define i64 @test(i64 %count, i64 %stop) {
entry:
  br label %header
header:
  %index = phi i64 [ 0, %entry ], [ %next, %latch ]
  %total = phi i64 [ 0, %entry ], [ %updated, %latch ]
  %done = icmp eq i64 %index, %count
  br i1 %done, label %exit, label %body
body:
  %parity = and i64 %index, 1
  %allocate = icmp eq i64 %parity, 1
  br i1 %allocate, label %allocation, label %latch
allocation:
  %buffer = call ptr @seq_alloc_atomic(i64 65536)
  store i8 42, ptr %buffer
  %value = call i8 @read(ptr %buffer)
  call void @seq_free(ptr nonnull %buffer)
  %extended = zext i8 %value to i64
  %sum = add i64 %total, %extended
  %finish = icmp eq i64 %index, %stop
  br i1 %finish, label %early, label %latch
latch:
  %updated = phi i64 [ %total, %body ], [ %sum, %allocation ]
  %next = add i64 %index, 1
  br label %header
early:
  call void @early_exit()
  ret i64 %sum
exit:
  ret i64 %total
}
""") as module:
        cfg = FunctionCFG(module.function("test"))
        frees = list(call_sites(cfg, "seq_free"))
        assert frees
        assert all(not cfg.loop_for(key) for key, _ in frees)
        _, alloc = next(call_sites(cfg, "seq_alloc_atomic"))
        assert any(user.is_a("PHINode") for user in alloc.users())



def test_does_not_hoist_free_across_exceptional_exit(optimize_ir):
    with optimize_ir("""
declare noalias ptr @seq_alloc_atomic(i64)
declare void @seq_free(ptr) nounwind
declare i8 @read(ptr nocapture) nofree
declare void @cleanup() nounwind
declare i32 @__gxx_personality_v0(...)

define i64 @test(i64 %count) personality ptr @__gxx_personality_v0 {
entry:
  br label %header
header:
  %index = phi i64 [ 0, %entry ], [ %next, %normal ]
  %total = phi i64 [ 0, %entry ], [ %updated, %normal ]
  %done = icmp eq i64 %index, %count
  br i1 %done, label %exit, label %body
body:
  %allocation = call ptr @seq_alloc_atomic(i64 65536)
  %value = invoke i8 @read(ptr %allocation) to label %normal unwind label %unwind
normal:
  call void @seq_free(ptr %allocation)
  %extended = zext i8 %value to i64
  %updated = add i64 %total, %extended
  %next = add i64 %index, 1
  br label %header
unwind:
  %exception = landingpad { ptr, i32 } cleanup
  call void @cleanup()
  resume { ptr, i32 } %exception
exit:
  ret i64 %total
}
""") as module:
        cfg = FunctionCFG(module.function("test"))
        key, alloc = next(call_sites(cfg, "seq_alloc_atomic"))
        assert cfg.loop_for(key)
        assert not any(user.is_a("PHINode") for user in alloc.users())



@pytest.mark.compiler
def test_reuses_openmp_thread_ids(llvm_library, codon_runtime, tmp_path):
    source = """
import openmp as omp

@export
def static_ids(data: Ptr[int], count: int):
    @par(num_threads=4)
    for index in range(count):
        data[index] = omp._get_gtid() + omp.get_thread_num()
    data[0] = omp.get_thread_num()

@export
def chunked_ids(data: Ptr[int], count: int):
    @par(schedule='static', chunk_size=7)
    for index in range(count):
        data[index] = omp._get_gtid() + omp.get_thread_num()

@export
def dynamic_ids(data: Ptr[int], count: int):
    @par(schedule='dynamic', chunk_size=7)
    for index in range(count):
        data[index] = omp._get_gtid() + omp.get_thread_num()

def items(count):
  for index in range(count):
    yield index

@export
def task_ids(data: Ptr[int], count: int):
  @par
  for index in items(count):
    data[index] = omp._get_gtid()
"""
    with optimize_source(llvm_library, codon_runtime, source, tmp_path / "allocation_phi_test.codon") as module:
        outlines = outside = 0
        for fn in module.functions():
            if fn._call("IsDeclaration", I, ()):
                continue
            outlined = "_loop_outline_template" in fn.name.split(".context.", 1)[0]
            outlines += outlined
            for block in fn.blocks():
                for inst in block.instructions():
                    if inst.is_call and (callee := inst.called_function) and callee.name in (
                        "__kmpc_global_thread_num", "omp_get_thread_num",
                    ):
                        assert not outlined, fn.name
                        outside += 1
        assert outlines == 5
        assert outside


def test_keeps_openmp_queries_without_stable_context(optimize_ir):
    with optimize_ir("""
declare void @__kmpc_fork_call(ptr, i32, ptr, ...)
declare ptr @__kmpc_omp_task_alloc(ptr, i32, i32, i64, i64, ptr, ptr)
declare i32 @__kmpc_global_thread_num(ptr)
declare i32 @omp_get_thread_num()
declare void @escape(ptr)

define private void @escaped(ptr %gtid, ptr %btid, ptr %result) noinline {
  %global = call i32 @__kmpc_global_thread_num(ptr null)
  %team = call i32 @omp_get_thread_num()
  %total = add i32 %global, %team
  store i32 %total, ptr %result
  ret void
}

define private i32 @untied(i32 %gtid, ptr %task) noinline {
  %global = call i32 @__kmpc_global_thread_num(ptr null)
  ret i32 %global
}

define void @launch(ptr %result) {
  call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr null, i32 1,
                                                ptr @escaped, ptr %result)
  call void @escape(ptr @escaped)
  %task = call ptr @__kmpc_omp_task_alloc(ptr null, i32 0, i32 0, i64 64,
                                        i64 0, ptr @untied, ptr null)
  call void @escape(ptr %task)
  ret void
}
""") as module:
        assert sum(
            1 for fn in module.functions() for block in fn.blocks() for inst in block.instructions()
            if inst.is_call and (callee := inst.called_function) and callee.name in (
                "__kmpc_global_thread_num", "omp_get_thread_num",
            )
        ) == 3
