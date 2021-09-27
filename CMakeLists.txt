cmake_minimum_required(VERSION 3.14)
project(Seq)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden -pedantic -Wall -Wno-return-type-c-linkage -Wno-gnu-zero-variadic-macro-arguments")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-limit-debug-info")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
include_directories(.)
include_directories(compiler)

# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(LLVM REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include(${CMAKE_SOURCE_DIR}/scripts/deps.cmake)

set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
if(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path;@loader_path/../lib/seq")
    set(STATIC_LIBCPP "")
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib/seq")
    set(STATIC_LIBCPP "-static-libstdc++")
endif()

add_executable(peg2cpp compiler/util/peg2cpp.cpp compiler/util/peglib.h)
target_link_libraries(peg2cpp Threads::Threads)
add_custom_command(
    OUTPUT  seq_rules.cpp
    COMMAND peg2cpp ${CMAKE_SOURCE_DIR}/compiler/parser/peg/grammar.peg seq_rules.cpp seq
    DEPENDS peg2cpp compiler/parser/peg/grammar.peg)
add_custom_command(
    OUTPUT  omp_rules.cpp
    COMMAND peg2cpp ${CMAKE_SOURCE_DIR}/compiler/parser/peg/openmp.peg omp_rules.cpp omp
    DEPENDS peg2cpp compiler/parser/peg/openmp.peg)

# Seq runtime library
set(SEQRT_FILES
    runtime/lib.h
    runtime/lib.cpp
    runtime/exc.cpp
    runtime/sw/ksw2.h
    runtime/sw/ksw2_extd2_sse.cpp
    runtime/sw/ksw2_exts2_sse.cpp
    runtime/sw/ksw2_extz2_sse.cpp
    runtime/sw/ksw2_gg2_sse.cpp
    runtime/sw/intersw.h
    runtime/sw/intersw.cpp)
add_library(seqrt SHARED ${SEQRT_FILES})
add_dependencies(seqrt bz2 liblzma zlibstatic gc htslib backtrace)
set_source_files_properties(runtime/sw/intersw.cpp PROPERTIES COMPILE_FLAGS -mavx)
target_include_directories(seqrt PRIVATE ${backtrace_SOURCE_DIR} "${gc_SOURCE_DIR}/include" runtime)
target_link_libraries(seqrt PRIVATE omp backtrace ${STATIC_LIBCPP} LLVMSupport)
if(APPLE)
    target_link_libraries(seqrt PRIVATE
        -Wl,-force_load,$<TARGET_FILE:bz2>
        -Wl,-force_load,$<TARGET_FILE:liblzma>
        -Wl,-force_load,$<TARGET_FILE:zlibstatic>
        -Wl,-force_load,$<TARGET_FILE:gc>
        -Wl,-force_load,$<TARGET_FILE:htslib> )
else()
    target_link_libraries(seqrt PRIVATE
        -Wl,--whole-archive
        $<TARGET_FILE:bz2>
        $<TARGET_FILE:liblzma>
        $<TARGET_FILE:zlibstatic>
        $<TARGET_FILE:gc>
        $<TARGET_FILE:htslib>
        -Wl,--no-whole-archive)
endif()
add_custom_command(TARGET seqrt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:omp> ${CMAKE_BINARY_DIR})

# Seq compiler library
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(SEQ_HPPFILES
    compiler/dsl/dsl.h
    compiler/dsl/plugins.h
    compiler/parser/ast.h
    compiler/parser/ast/expr.h
    compiler/parser/ast/stmt.h
    compiler/parser/ast/types.h
    compiler/parser/cache.h
    compiler/parser/common.h
    compiler/parser/ctx.h
    compiler/parser/peg/peg.h
    compiler/parser/peg/rules.h
    compiler/parser/parser.h
    compiler/parser/visitors/doc/doc.h
    compiler/parser/visitors/format/format.h
    compiler/parser/visitors/simplify/simplify.h
    compiler/parser/visitors/simplify/simplify_ctx.h
    compiler/parser/visitors/translate/translate.h
    compiler/parser/visitors/translate/translate_ctx.h
    compiler/parser/visitors/typecheck/typecheck.h
    compiler/parser/visitors/typecheck/typecheck_ctx.h
    compiler/parser/visitors/visitor.h
    compiler/seq/pipeline.h
    compiler/seq/revcomp.h
    compiler/seq/seq.h
    compiler/sir/analyze/analysis.h
    compiler/sir/analyze/dataflow/cfg.h
    compiler/sir/analyze/dataflow/dominator.h
    compiler/sir/analyze/dataflow/reaching.h
    compiler/sir/analyze/module/global_vars.h
    compiler/sir/analyze/module/side_effect.h
    compiler/sir/attribute.h
    compiler/sir/base.h
    compiler/sir/const.h
    compiler/sir/dsl/codegen.h
    compiler/sir/dsl/nodes.h
    compiler/sir/flow.h
    compiler/sir/func.h
    compiler/sir/instr.h
    compiler/sir/llvm/coro/CoroCleanup.h
    compiler/sir/llvm/coro/CoroEarly.h
    compiler/sir/llvm/coro/CoroElide.h
    compiler/sir/llvm/coro/CoroInstr.h
    compiler/sir/llvm/coro/CoroInternal.h
    compiler/sir/llvm/coro/CoroSplit.h
    compiler/sir/llvm/coro/Coroutines.h
    compiler/sir/llvm/llvisitor.h
    compiler/sir/llvm/llvm.h
    compiler/sir/module.h
    compiler/sir/sir.h
    compiler/sir/transform/cleanup/canonical.h
    compiler/sir/transform/cleanup/dead_code.h
    compiler/sir/transform/cleanup/global_demote.h
    compiler/sir/transform/cleanup/replacer.h
    compiler/sir/transform/folding/const_fold.h
    compiler/sir/transform/folding/const_prop.h
    compiler/sir/transform/folding/folding.h
    compiler/sir/transform/folding/rule.h
    compiler/sir/transform/lowering/imperative.h
    compiler/sir/transform/lowering/pipeline.h
    compiler/sir/transform/manager.h
    compiler/sir/transform/parallel/openmp.h
    compiler/sir/transform/parallel/schedule.h
    compiler/sir/transform/pass.h
    compiler/sir/transform/pythonic/dict.h
    compiler/sir/transform/pythonic/io.h
    compiler/sir/transform/pythonic/str.h
    compiler/sir/transform/rewrite.h
    compiler/sir/types/types.h
    compiler/sir/util/cloning.h
    compiler/sir/util/context.h
    compiler/sir/util/format.h
    compiler/sir/util/inlining.h
    compiler/sir/util/irtools.h
    compiler/sir/util/iterators.h
    compiler/sir/util/matching.h
    compiler/sir/util/operator.h
    compiler/sir/util/outlining.h
    compiler/sir/util/packs.h
    compiler/sir/util/visitor.h
    compiler/sir/value.h
    compiler/sir/var.h
    compiler/util/common.h
    compiler/util/peglib.h
    compiler/util/fmt/chrono.h
    compiler/util/fmt/color.h
    compiler/util/fmt/compile.h
    compiler/util/fmt/core.h
    compiler/util/fmt/format-inl.h
    compiler/util/fmt/format.h
    compiler/util/fmt/locale.h
    compiler/util/fmt/ostream.h
    compiler/util/fmt/posix.h
    compiler/util/fmt/printf.h
    compiler/util/fmt/ranges.h)
set(SEQ_CPPFILES
    compiler/dsl/plugins.cpp
    compiler/parser/ast/expr.cpp
    compiler/parser/ast/stmt.cpp
    compiler/parser/ast/types.cpp
    compiler/parser/cache.cpp
    compiler/parser/common.cpp
    compiler/parser/peg/peg.cpp
    compiler/parser/parser.cpp
    compiler/parser/visitors/doc/doc.cpp
    compiler/parser/visitors/format/format.cpp
    compiler/parser/visitors/simplify/simplify.cpp
    compiler/parser/visitors/simplify/simplify_ctx.cpp
    compiler/parser/visitors/simplify/simplify_expr.cpp
    compiler/parser/visitors/simplify/simplify_stmt.cpp
    compiler/parser/visitors/translate/translate.cpp
    compiler/parser/visitors/translate/translate_ctx.cpp
    compiler/parser/visitors/typecheck/typecheck.cpp
    compiler/parser/visitors/typecheck/typecheck_ctx.cpp
    compiler/parser/visitors/typecheck/typecheck_expr.cpp
    compiler/parser/visitors/typecheck/typecheck_infer.cpp
    compiler/parser/visitors/typecheck/typecheck_stmt.cpp
    compiler/parser/visitors/visitor.cpp
    compiler/seq/pipeline.cpp
    compiler/seq/revcomp.cpp
    compiler/seq/seq.cpp
    compiler/sir/attribute.cpp
    compiler/sir/analyze/analysis.cpp
    compiler/sir/analyze/dataflow/cfg.cpp
    compiler/sir/analyze/dataflow/dominator.cpp
    compiler/sir/analyze/dataflow/reaching.cpp
    compiler/sir/analyze/module/global_vars.cpp
    compiler/sir/analyze/module/side_effect.cpp
    compiler/sir/base.cpp
    compiler/sir/const.cpp
    compiler/sir/dsl/nodes.cpp
    compiler/sir/flow.cpp
    compiler/sir/func.cpp
    compiler/sir/instr.cpp
    compiler/sir/llvm/coro/CoroCleanup.cpp
    compiler/sir/llvm/coro/CoroEarly.cpp
    compiler/sir/llvm/coro/CoroElide.cpp
    compiler/sir/llvm/coro/CoroFrame.cpp
    compiler/sir/llvm/coro/CoroSplit.cpp
    compiler/sir/llvm/coro/Coroutines.cpp
    compiler/sir/llvm/llvisitor.cpp
    compiler/sir/module.cpp
    compiler/sir/transform/cleanup/canonical.cpp
    compiler/sir/transform/cleanup/dead_code.cpp
    compiler/sir/transform/cleanup/global_demote.cpp
    compiler/sir/transform/cleanup/replacer.cpp
    compiler/sir/transform/folding/const_fold.cpp
    compiler/sir/transform/folding/const_prop.cpp
    compiler/sir/transform/folding/folding.cpp
    compiler/sir/transform/lowering/imperative.cpp
    compiler/sir/transform/lowering/pipeline.cpp
    compiler/sir/transform/manager.cpp
    compiler/sir/transform/parallel/openmp.cpp
    compiler/sir/transform/parallel/schedule.cpp
    compiler/sir/transform/pass.cpp
    compiler/sir/transform/pythonic/dict.cpp
    compiler/sir/transform/pythonic/io.cpp
    compiler/sir/transform/pythonic/str.cpp
    compiler/sir/types/types.cpp
    compiler/sir/util/cloning.cpp
    compiler/sir/util/format.cpp
    compiler/sir/util/inlining.cpp
    compiler/sir/util/irtools.cpp
    compiler/sir/util/matching.cpp
    compiler/sir/util/outlining.cpp
    compiler/sir/util/visitor.cpp
    compiler/sir/value.cpp
    compiler/sir/var.cpp
    compiler/util/common.cpp
    compiler/util/fmt/format.cpp)
add_library(seq SHARED ${SEQ_HPPFILES})
target_sources(seq PRIVATE ${SEQ_CPPFILES} seq_rules.cpp omp_rules.cpp)
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set_source_files_properties(seq_rules.cpp compiler/parser/peg/peg.cpp PROPERTIES COMPILE_FLAGS "-O2")
endif()
llvm_map_components_to_libnames(LLVM_LIBS
    AllTargetsAsmParsers
    AllTargetsCodeGens
    AllTargetsDescs
    AllTargetsInfos
    AggressiveInstCombine
    Analysis
    AsmParser
    BitWriter
    CodeGen
    Core
    Extensions
    IPO
    IRReader
    InstCombine
    Instrumentation
    MC
    MCJIT
    ObjCARCOpts
    OrcJIT
    Remarks
    ScalarOpts
    Support
    Target
    TransformUtils
    Vectorize
    Passes)
if(APPLE)
    target_link_libraries(seq PRIVATE ${LLVM_LIBS} dl seqrt)
else()
    target_link_libraries(seq PRIVATE ${STATIC_LIBCPP} ${LLVM_LIBS} dl seqrt)
endif()

# Seq command-line tool
add_executable(seqc runtime/main.cpp)
target_link_libraries(seqc ${STATIC_LIBCPP} seq Threads::Threads)

# Seq test
# Download and unpack googletest at configure time
include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()
set(SEQ_TEST_CPPFILES
    test/main.cpp
    test/sir/analyze/dominator.cpp
    test/sir/analyze/reaching.cpp
    test/sir/base.cpp
    test/sir/constant.cpp
    test/sir/flow.cpp
    test/sir/func.cpp
    test/sir/instr.cpp
    test/sir/module.cpp
    test/sir/transform/manager.cpp
    test/sir/types/types.cpp
    test/sir/util/matching.cpp
    test/sir/value.cpp
    test/sir/var.cpp
    test/types.cpp)
add_executable(seqtest ${SEQ_TEST_CPPFILES})
target_include_directories(seqtest PRIVATE test/sir "${gc_SOURCE_DIR}/include")
target_link_libraries(seqtest seq seqrt gtest_main)
target_compile_definitions(seqtest PRIVATE TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test")

include(GoogleTest)
gtest_discover_tests(seqtest)
enable_testing()
