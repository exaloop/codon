cmake_minimum_required(VERSION 3.14)
project(Codon VERSION "0.11.1"
              HOMEPAGE_URL "https://github.com/exaloop/codon"
              DESCRIPTION "high-performance, extensible Python compiler")
configure_file("${PROJECT_SOURCE_DIR}/cmake/config.h.in"
               "${PROJECT_SOURCE_DIR}/codon/config/config.h")
configure_file("${PROJECT_SOURCE_DIR}/cmake/config.py.in"
               "${PROJECT_SOURCE_DIR}/docs/sphinx/config/config.py")

option(CODON_JUPYTER "build Codon Jupyter server" OFF)
if(CODON_JUPYTER)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/extra/jupyter/share/jupyter/kernels/codon/kernel.json.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/extra/jupyter/share/jupyter/kernels/codon/kernel.json"
    )
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -fvisibility-inlines-hidden -Wno-return-type-c-linkage -Wno-gnu-zero-variadic-macro-arguments -Wno-deprecated-declarations")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-return-type")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g")
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-limit-debug-info")
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
include_directories(.)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(LLVM REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include(${CMAKE_SOURCE_DIR}/cmake/deps.cmake)

set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
if(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path;@loader_path/../lib/codon")
    set(STATIC_LIBCPP "")
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib/codon")
    set(STATIC_LIBCPP "-static-libstdc++")
endif()

add_executable(peg2cpp codon/util/peg2cpp.cpp codon/util/cpp-peglib/peglib.h)
target_link_libraries(peg2cpp Threads::Threads)
add_custom_command(
    OUTPUT  codon_rules.cpp
    COMMAND peg2cpp ${CMAKE_SOURCE_DIR}/codon/parser/peg/grammar.peg codon_rules.cpp codon
    DEPENDS peg2cpp codon/parser/peg/grammar.peg)
add_custom_command(
    OUTPUT  omp_rules.cpp
    COMMAND peg2cpp ${CMAKE_SOURCE_DIR}/codon/parser/peg/openmp.peg omp_rules.cpp omp
    DEPENDS peg2cpp codon/parser/peg/openmp.peg)

# Codon runtime library
set(CODONRT_FILES
    codon/runtime/lib.h
    codon/runtime/lib.cpp
    codon/runtime/exc.cpp)
add_library(codonrt SHARED ${CODONRT_FILES})
add_dependencies(codonrt zlibstatic gc backtrace bz2 liblzma)
target_include_directories(codonrt PRIVATE ${backtrace_SOURCE_DIR} "${gc_SOURCE_DIR}/include" runtime)
target_link_libraries(codonrt PRIVATE omp backtrace ${STATIC_LIBCPP} LLVMSupport)
if(APPLE)
    target_link_libraries(codonrt PRIVATE
        -Wl,-force_load,$<TARGET_FILE:zlibstatic>
        -Wl,-force_load,$<TARGET_FILE:gc>
        -Wl,-force_load,$<TARGET_FILE:bz2>
        -Wl,-force_load,$<TARGET_FILE:liblzma> )
else()
    target_link_libraries(codonrt PRIVATE
        -Wl,--whole-archive
        $<TARGET_FILE:zlibstatic>
        $<TARGET_FILE:gc>
        $<TARGET_FILE:bz2>
        $<TARGET_FILE:liblzma>
        -Wl,--no-whole-archive)
endif()
if(ASAN)
    target_compile_options(codonrt PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address" "-fsanitize-recover=address")
    target_link_libraries(codonrt PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address" "-fsanitize-recover=address")
endif()
add_custom_command(TARGET codonrt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:omp> ${CMAKE_BINARY_DIR})

# Codon compiler library
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
set(CODON_HPPFILES
    codon/compiler/compiler.h
    codon/compiler/debug_listener.h
    codon/compiler/engine.h
    codon/compiler/error.h
    codon/compiler/jit.h
    codon/compiler/memory_manager.h
    codon/dsl/dsl.h
    codon/dsl/plugins.h
    codon/parser/ast.h
    codon/parser/ast/expr.h
    codon/parser/ast/stmt.h
    codon/parser/ast/types.h
    codon/parser/cache.h
    codon/parser/common.h
    codon/parser/ctx.h
    codon/parser/peg/peg.h
    codon/parser/peg/rules.h
    codon/parser/visitors/doc/doc.h
    codon/parser/visitors/format/format.h
    codon/parser/visitors/simplify/simplify.h
    codon/parser/visitors/simplify/simplify_ctx.h
    codon/parser/visitors/translate/translate.h
    codon/parser/visitors/translate/translate_ctx.h
    codon/parser/visitors/typecheck/typecheck.h
    codon/parser/visitors/typecheck/typecheck_ctx.h
    codon/parser/visitors/visitor.h
    codon/sir/analyze/analysis.h
    codon/sir/analyze/dataflow/cfg.h
    codon/sir/analyze/dataflow/dominator.h
    codon/sir/analyze/dataflow/reaching.h
    codon/sir/analyze/module/global_vars.h
    codon/sir/analyze/module/side_effect.h
    codon/sir/attribute.h
    codon/sir/base.h
    codon/sir/const.h
    codon/sir/dsl/codegen.h
    codon/sir/dsl/nodes.h
    codon/sir/flow.h
    codon/sir/func.h
    codon/sir/instr.h
    codon/sir/llvm/coro/CoroCleanup.h
    codon/sir/llvm/coro/CoroEarly.h
    codon/sir/llvm/coro/CoroElide.h
    codon/sir/llvm/coro/CoroInstr.h
    codon/sir/llvm/coro/CoroInternal.h
    codon/sir/llvm/coro/CoroSplit.h
    codon/sir/llvm/coro/Coroutines.h
    codon/sir/llvm/llvisitor.h
    codon/sir/llvm/llvm.h
    codon/sir/llvm/optimize.h
    codon/sir/module.h
    codon/sir/sir.h
    codon/sir/transform/cleanup/canonical.h
    codon/sir/transform/cleanup/dead_code.h
    codon/sir/transform/cleanup/global_demote.h
    codon/sir/transform/cleanup/replacer.h
    codon/sir/transform/folding/const_fold.h
    codon/sir/transform/folding/const_prop.h
    codon/sir/transform/folding/folding.h
    codon/sir/transform/folding/rule.h
    codon/sir/transform/lowering/imperative.h
    codon/sir/transform/lowering/pipeline.h
    codon/sir/transform/manager.h
    codon/sir/transform/parallel/openmp.h
    codon/sir/transform/parallel/schedule.h
    codon/sir/transform/pass.h
    codon/sir/transform/pythonic/dict.h
    codon/sir/transform/pythonic/io.h
    codon/sir/transform/pythonic/str.h
    codon/sir/transform/rewrite.h
    codon/sir/types/types.h
    codon/sir/util/cloning.h
    codon/sir/util/context.h
    codon/sir/util/escape.h
    codon/sir/util/format.h
    codon/sir/util/inlining.h
    codon/sir/util/irtools.h
    codon/sir/util/iterators.h
    codon/sir/util/matching.h
    codon/sir/util/operator.h
    codon/sir/util/outlining.h
    codon/sir/util/packs.h
    codon/sir/util/side_effect.h
    codon/sir/util/visitor.h
    codon/sir/value.h
    codon/sir/var.h
    codon/util/common.h
    codon/util/cpp-peglib/peglib.h
    codon/util/fmt/chrono.h
    codon/util/fmt/color.h
    codon/util/fmt/compile.h
    codon/util/fmt/core.h
    codon/util/fmt/format-inl.h
    codon/util/fmt/format.h
    codon/util/fmt/locale.h
    codon/util/fmt/ostream.h
    codon/util/fmt/posix.h
    codon/util/fmt/printf.h
    codon/util/fmt/ranges.h
    codon/util/semver/semver.h
    codon/util/toml++/toml.h
    codon/util/toml++/toml_default_formatter.hpp
    codon/util/toml++/toml_node.hpp
    codon/util/toml++/toml_preprocessor.h
    codon/util/toml++/toml_utf8_streams.hpp
    codon/util/toml++/toml_array.h
    codon/util/toml++/toml_formatter.h
    codon/util/toml++/toml_node_view.h
    codon/util/toml++/toml_print_to_stream.h
    codon/util/toml++/toml_value.h
    codon/util/toml++/toml_array.hpp
    codon/util/toml++/toml_instantiations.hpp
    codon/util/toml++/toml_parse_error.h
    codon/util/toml++/toml_table.h
    codon/util/toml++/toml_version.h
    codon/util/toml++/toml_common.h
    codon/util/toml++/toml_json_formatter.h
    codon/util/toml++/toml_parse_result.h
    codon/util/toml++/toml_table.hpp
    codon/util/toml++/toml_date_time.h
    codon/util/toml++/toml_json_formatter.hpp
    codon/util/toml++/toml_parser.h
    codon/util/toml++/toml_utf8.h
    codon/util/toml++/toml_default_formatter.h
    codon/util/toml++/toml_node.h
    codon/util/toml++/toml_parser.hpp
    codon/util/toml++/toml_utf8_streams.h
    extra/jupyter/jupyter.h)
set(CODON_CPPFILES
    codon/compiler/compiler.cpp
    codon/compiler/debug_listener.cpp
    codon/compiler/engine.cpp
    codon/compiler/error.cpp
    codon/compiler/jit.cpp
    codon/compiler/memory_manager.cpp
    codon/dsl/plugins.cpp
    codon/parser/ast/expr.cpp
    codon/parser/ast/stmt.cpp
    codon/parser/ast/types.cpp
    codon/parser/cache.cpp
    codon/parser/common.cpp
    codon/parser/peg/peg.cpp
    codon/parser/visitors/doc/doc.cpp
    codon/parser/visitors/format/format.cpp
    codon/parser/visitors/simplify/simplify.cpp
    codon/parser/visitors/simplify/simplify_ctx.cpp
    codon/parser/visitors/simplify/simplify_expr.cpp
    codon/parser/visitors/simplify/simplify_stmt.cpp
    codon/parser/visitors/translate/translate.cpp
    codon/parser/visitors/translate/translate_ctx.cpp
    codon/parser/visitors/typecheck/typecheck.cpp
    codon/parser/visitors/typecheck/typecheck_ctx.cpp
    codon/parser/visitors/typecheck/typecheck_expr.cpp
    codon/parser/visitors/typecheck/typecheck_infer.cpp
    codon/parser/visitors/typecheck/typecheck_stmt.cpp
    codon/parser/visitors/visitor.cpp
    codon/sir/attribute.cpp
    codon/sir/analyze/analysis.cpp
    codon/sir/analyze/dataflow/cfg.cpp
    codon/sir/analyze/dataflow/dominator.cpp
    codon/sir/analyze/dataflow/reaching.cpp
    codon/sir/analyze/module/global_vars.cpp
    codon/sir/analyze/module/side_effect.cpp
    codon/sir/base.cpp
    codon/sir/const.cpp
    codon/sir/dsl/nodes.cpp
    codon/sir/flow.cpp
    codon/sir/func.cpp
    codon/sir/instr.cpp
    codon/sir/llvm/coro/CoroCleanup.cpp
    codon/sir/llvm/coro/CoroEarly.cpp
    codon/sir/llvm/coro/CoroElide.cpp
    codon/sir/llvm/coro/CoroFrame.cpp
    codon/sir/llvm/coro/CoroSplit.cpp
    codon/sir/llvm/coro/Coroutines.cpp
    codon/sir/llvm/llvisitor.cpp
    codon/sir/llvm/optimize.cpp
    codon/sir/module.cpp
    codon/sir/transform/cleanup/canonical.cpp
    codon/sir/transform/cleanup/dead_code.cpp
    codon/sir/transform/cleanup/global_demote.cpp
    codon/sir/transform/cleanup/replacer.cpp
    codon/sir/transform/folding/const_fold.cpp
    codon/sir/transform/folding/const_prop.cpp
    codon/sir/transform/folding/folding.cpp
    codon/sir/transform/lowering/imperative.cpp
    codon/sir/transform/lowering/pipeline.cpp
    codon/sir/transform/manager.cpp
    codon/sir/transform/parallel/openmp.cpp
    codon/sir/transform/parallel/schedule.cpp
    codon/sir/transform/pass.cpp
    codon/sir/transform/pythonic/dict.cpp
    codon/sir/transform/pythonic/io.cpp
    codon/sir/transform/pythonic/str.cpp
    codon/sir/types/types.cpp
    codon/sir/util/cloning.cpp
    codon/sir/util/escape.cpp
    codon/sir/util/format.cpp
    codon/sir/util/inlining.cpp
    codon/sir/util/irtools.cpp
    codon/sir/util/matching.cpp
    codon/sir/util/outlining.cpp
    codon/sir/util/visitor.cpp
    codon/sir/value.cpp
    codon/sir/var.cpp
    codon/util/common.cpp
    codon/util/fmt/format.cpp
    extra/jupyter/jupyter.cpp)
add_library(codonc SHARED ${CODON_HPPFILES})
target_sources(codonc PRIVATE ${CODON_CPPFILES} codon_rules.cpp omp_rules.cpp)
if(CODON_JUPYTER)
    add_compile_definitions(CODON_JUPYTER)
    add_dependencies(codonc xeus-static nlohmann_json)
    target_link_libraries(codonc PRIVATE xeus-static)
endif()
if(ASAN)
    target_compile_options(codonc PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address" "-fsanitize-recover=address")
    target_link_libraries(codonc PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address" "-fsanitize-recover=address")
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set_source_files_properties(codon_rules.cpp codon/parser/peg/peg.cpp PROPERTIES COMPILE_FLAGS "-O2")
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
    Symbolize
    Target
    TransformUtils
    Vectorize
    Passes)
if(APPLE)
    target_link_libraries(codonc PRIVATE ${LLVM_LIBS} dl codonrt)
else()
    target_link_libraries(codonc PRIVATE ${STATIC_LIBCPP} ${LLVM_LIBS} dl codonrt)
endif()

# Gather headers
add_custom_target(
    headers
    ALL
    COMMENT "Collecting headers"
    BYPRODUCTS "${CMAKE_BINARY_DIR}/include"
    VERBATIM
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_BINARY_DIR}/include/codon"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/codon" "${CMAKE_BINARY_DIR}/include/codon"
    COMMAND find "${CMAKE_BINARY_DIR}/include" -type f ! -name "*.h" -exec rm {} \\;
)
add_dependencies(headers codonrt codonc)

# Codon command-line tool
add_executable(codon codon/app/main.cpp)
target_link_libraries(codon PUBLIC ${STATIC_LIBCPP} codonc Threads::Threads)

# Codon test
# Download and unpack googletest at configure time
include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()
set(CODON_TEST_CPPFILES
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
add_executable(codon_test ${CODON_TEST_CPPFILES})
target_include_directories(codon_test PRIVATE test/sir "${gc_SOURCE_DIR}/include")
target_link_libraries(codon_test codonc codonrt gtest_main)
target_compile_definitions(codon_test PRIVATE TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test")

