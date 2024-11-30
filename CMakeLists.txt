cmake_minimum_required(VERSION 3.14)
project(
  Codon
  VERSION "0.17.0"
  HOMEPAGE_URL "https://github.com/exaloop/codon"
  DESCRIPTION "high-performance, extensible Python compiler")
set(CODON_JIT_PYTHON_VERSION "0.2.0")
configure_file("${PROJECT_SOURCE_DIR}/cmake/config.h.in"
               "${PROJECT_SOURCE_DIR}/codon/config/config.h")
configure_file("${PROJECT_SOURCE_DIR}/cmake/config.py.in"
               "${PROJECT_SOURCE_DIR}/jit/codon/version.py")

option(CODON_GPU "build Codon GPU backend" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -pedantic -fvisibility-inlines-hidden -Wno-return-type-c-linkage -Wno-gnu-zero-variadic-macro-arguments -Wno-deprecated-declarations"
  )
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-return-type")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-limit-debug-info")
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
include_directories(.)

set(APPLE_ARM OFF)
if (APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
  set(APPLE_ARM ON)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(LLVM REQUIRED)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

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

add_executable(peg2cpp codon/util/peg2cpp.cpp)
target_include_directories(peg2cpp PRIVATE ${peglib_SOURCE_DIR})
target_link_libraries(peg2cpp PRIVATE Threads::Threads fmt)
add_custom_command(
  OUTPUT codon_rules.cpp
  COMMAND peg2cpp ${CMAKE_SOURCE_DIR}/codon/parser/peg/grammar.peg
          codon_rules.cpp codon
  DEPENDS peg2cpp codon/parser/peg/grammar.peg)
add_custom_command(
  OUTPUT omp_rules.cpp
  COMMAND peg2cpp ${CMAKE_SOURCE_DIR}/codon/parser/peg/openmp.peg omp_rules.cpp
          omp
  DEPENDS peg2cpp codon/parser/peg/openmp.peg)

# Codon Jupyter library
set(CODON_JUPYTER_FILES codon/util/jupyter.h codon/util/jupyter.cpp)
add_library(codon_jupyter SHARED ${CODON_JUPYTER_FILES})

# Codon runtime library
set(CODONRT_FILES codon/runtime/lib.hpp codon/runtime/lib.cpp
                  codon/runtime/re.cpp codon/runtime/exc.cpp
                  codon/runtime/gpu.cpp)
add_library(codonrt SHARED ${CODONRT_FILES})
add_dependencies(codonrt zlibstatic gc backtrace bz2 liblzma re2 fast_float)
target_include_directories(codonrt PRIVATE ${backtrace_SOURCE_DIR}
                                           ${re2_SOURCE_DIR}
                                           "${gc_SOURCE_DIR}/include"
                                           "${fast_float_SOURCE_DIR}/include" runtime)
target_link_libraries(codonrt PRIVATE fmt omp backtrace ${STATIC_LIBCPP}
                                      LLVMSupport)
if(APPLE)
  target_link_libraries(
    codonrt
    PRIVATE -Wl,-force_load,$<TARGET_FILE:zlibstatic>
            -Wl,-force_load,$<TARGET_FILE:gc>
            -Wl,-force_load,$<TARGET_FILE:bz2>
            -Wl,-force_load,$<TARGET_FILE:liblzma>
            -Wl,-force_load,$<TARGET_FILE:re2>)
else()
  target_link_libraries(
    codonrt
    PRIVATE -Wl,--whole-archive $<TARGET_FILE:zlibstatic> $<TARGET_FILE:gc>
            $<TARGET_FILE:bz2> $<TARGET_FILE:liblzma> $<TARGET_FILE:re2>
            -Wl,--no-whole-archive)
endif()
if(ASAN)
  target_compile_options(
    codonrt PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address"
                    "-fsanitize-recover=address")
  target_link_libraries(
    codonrt PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address"
                    "-fsanitize-recover=address")
endif()
if(CODON_GPU)
  add_compile_definitions(CODON_GPU)
  find_package(CUDAToolkit REQUIRED)
  target_link_libraries(codonrt PRIVATE CUDA::cudart_static CUDA::cuda_driver)
endif()
add_custom_command(
  TARGET codonrt
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:omp>
          ${CMAKE_BINARY_DIR})

# Codon compiler library
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
set(CODON_HPPFILES
    codon/compiler/compiler.hpp
    codon/compiler/debug_listener.hpp
    codon/compiler/engine.hpp
    codon/compiler/error.hpp
    codon/compiler/jit.hpp
    codon/compiler/memory_manager.hpp
    codon/dsl/dsl.hpp
    codon/dsl/plugins.hpp
    codon/parser/ast.hpp
    codon/parser/ast/expr.hpp
    codon/parser/ast/stmt.hpp
    codon/parser/ast/types.h
    codon/parser/ast/types/type.hpp
    codon/parser/ast/types/link.hpp
    codon/parser/ast/types/class.hpp
    codon/parser/ast/types/function.hpp
    codon/parser/ast/types/union.hpp
    codon/parser/ast/types/static.hpp
    codon/parser/ast/types/traits.hpp
    codon/parser/cache.hpp
    codon/parser/common.hpp
    codon/parser/ctx.hpp
    codon/parser/peg/peg.hpp
    codon/parser/peg/rules.hpp
    codon/parser/visitors/doc/doc.hpp
    codon/parser/visitors/format/format.hpp
    codon/parser/visitors/simplify/simplify.hpp
    codon/parser/visitors/simplify/ctx.hpp
    codon/parser/visitors/translate/translate.h
    codon/parser/visitors/translate/translate_ctx.hpp
    codon/parser/visitors/typecheck/typecheck.h
    codon/parser/visitors/typecheck/ctx.hpp
    codon/parser/visitors/visitor.h
    codon/cir/analyze/analysis.hpp
    codon/cir/analyze/dataflow/capture.hpp
    codon/cir/analyze/dataflow/cfg.hpp
    codon/cir/analyze/dataflow/dominator.hpp
    codon/cir/analyze/dataflow/reaching.hpp
    codon/cir/analyze/module/global_vars.hpp
    codon/cir/analyze/module/side_effect.hpp
    codon/cir/attribute.hpp
    codon/cir/base.hpp
    codon/cir/const.hpp
    codon/cir/dsl/codegen.hpp
    codon/cir/dsl/nodes.hpp
    codon/cir/flow.hpp
    codon/cir/func.hpp
    codon/cir/instr.hpp
    codon/cir/llvm/gpu.hpp
    codon/cir/llvm/llvisitor.hpp
    codon/cir/llvm/llvm.hpp
    codon/cir/llvm/optimize.hpp
    codon/cir/module.hpp
    codon/cir/pyextension.hpp
    codon/cir/cir.hpp
    codon/cir/transform/cleanup/canonical.hpp
    codon/cir/transform/cleanup/dead_code.hpp
    codon/cir/transform/cleanup/global_demote.hpp
    codon/cir/transform/cleanup/replacer.hpp
    codon/cir/transform/folding/const_fold.hpp
    codon/cir/transform/folding/const_prop.hpp
    codon/cir/transform/folding/folding.hpp
    codon/cir/transform/folding/rule.hpp
    codon/cir/transform/lowering/imperative.hpp
    codon/cir/transform/lowering/pipeline.hpp
    codon/cir/transform/manager.h
    codon/cir/transform/parallel/openmp.hpp
    codon/cir/transform/parallel/schedule.hpp
    codon/cir/transform/pass.hpp
    codon/cir/transform/pythonic/dict.hpp
    codon/cir/transform/pythonic/generator.hpp
    codon/cir/transform/pythonic/io.hpp
    codon/cir/transform/pythonic/list.hpp
    codon/cir/transform/pythonic/str.hpp
    codon/cir/transform/rewrite.hpp
    codon/cir/types/types.hpp
    codon/cir/util/cloning.hpp
    codon/cir/util/format.hpp
    codon/cir/util/inlining.hpp
    codon/cir/util/irtools.hpp
    codon/cir/util/iterators.hpp
    codon/cir/util/matching.hpp
    codon/cir/util/operator.hpp
    codon/cir/util/outlining.hpp
    codon/cir/util/packs.hpp
    codon/cir/util/side_effect.hpp
    codon/cir/util/visitor.hpp
    codon/cir/value.hpp
    codon/cir/var.hpp
    codon/util/common.hpp
    codon/compiler/jit_extern.h
    )
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
    codon/parser/ast/types/type.cpp
    codon/parser/ast/types/link.cpp
    codon/parser/ast/types/class.cpp
    codon/parser/ast/types/function.cpp
    codon/parser/ast/types/union.cpp
    codon/parser/ast/types/static.cpp
    codon/parser/ast/types/traits.cpp
    codon/parser/cache.cpp
    codon/parser/common.cpp
    codon/parser/peg/peg.cpp
    codon/parser/visitors/doc/doc.cpp
    codon/parser/visitors/format/format.cpp
    codon/parser/visitors/simplify/simplify.cpp
    codon/parser/visitors/simplify/ctx.cpp
    codon/parser/visitors/simplify/assign.cpp
    codon/parser/visitors/simplify/basic.cpp
    codon/parser/visitors/simplify/call.cpp
    codon/parser/visitors/simplify/class.cpp
    codon/parser/visitors/simplify/collections.cpp
    codon/parser/visitors/simplify/cond.cpp
    codon/parser/visitors/simplify/function.cpp
    codon/parser/visitors/simplify/access.cpp
    codon/parser/visitors/simplify/import.cpp
    codon/parser/visitors/simplify/loops.cpp
    codon/parser/visitors/simplify/op.cpp
    codon/parser/visitors/simplify/error.cpp
    codon/parser/visitors/translate/translate.cpp
    codon/parser/visitors/translate/translate_ctx.cpp
    codon/parser/visitors/typecheck/typecheck.cpp
    codon/parser/visitors/typecheck/infer.cpp
    codon/parser/visitors/typecheck/ctx.cpp
    codon/parser/visitors/typecheck/assign.cpp
    codon/parser/visitors/typecheck/basic.cpp
    codon/parser/visitors/typecheck/call.cpp
    codon/parser/visitors/typecheck/class.cpp
    codon/parser/visitors/typecheck/collections.cpp
    codon/parser/visitors/typecheck/cond.cpp
    codon/parser/visitors/typecheck/function.cpp
    codon/parser/visitors/typecheck/access.cpp
    codon/parser/visitors/typecheck/loops.cpp
    codon/parser/visitors/typecheck/op.cpp
    codon/parser/visitors/typecheck/error.cpp
    codon/parser/visitors/visitor.cpp
    codon/cir/attribute.cpp
    codon/cir/analyze/analysis.cpp
    codon/cir/analyze/dataflow/capture.cpp
    codon/cir/analyze/dataflow/cfg.cpp
    codon/cir/analyze/dataflow/dominator.cpp
    codon/cir/analyze/dataflow/reaching.cpp
    codon/cir/analyze/module/global_vars.cpp
    codon/cir/analyze/module/side_effect.cpp
    codon/cir/base.cpp
    codon/cir/const.cpp
    codon/cir/dsl/nodes.cpp
    codon/cir/flow.cpp
    codon/cir/func.cpp
    codon/cir/instr.cpp
    codon/cir/llvm/gpu.cpp
    codon/cir/llvm/llvisitor.cpp
    codon/cir/llvm/optimize.cpp
    codon/cir/module.cpp
    codon/cir/transform/cleanup/canonical.cpp
    codon/cir/transform/cleanup/dead_code.cpp
    codon/cir/transform/cleanup/global_demote.cpp
    codon/cir/transform/cleanup/replacer.cpp
    codon/cir/transform/folding/const_fold.cpp
    codon/cir/transform/folding/const_prop.cpp
    codon/cir/transform/folding/folding.cpp
    codon/cir/transform/lowering/imperative.cpp
    codon/cir/transform/lowering/pipeline.cpp
    codon/cir/transform/manager.cpp
    codon/cir/transform/parallel/openmp.cpp
    codon/cir/transform/parallel/schedule.cpp
    codon/cir/transform/pass.cpp
    codon/cir/transform/pythonic/dict.cpp
    codon/cir/transform/pythonic/generator.cpp
    codon/cir/transform/pythonic/io.cpp
    codon/cir/transform/pythonic/list.cpp
    codon/cir/transform/pythonic/str.cpp
    codon/cir/types/types.cpp
    codon/cir/util/cloning.cpp
    codon/cir/util/format.cpp
    codon/cir/util/inlining.cpp
    codon/cir/util/irtools.cpp
    codon/cir/util/matching.cpp
    codon/cir/util/outlining.cpp
    codon/cir/util/side_effect.cpp
    codon/cir/util/visitor.cpp
    codon/cir/value.cpp
    codon/cir/var.cpp
    codon/util/common.cpp)
add_library(codonc SHARED ${CODON_HPPFILES})
target_include_directories(codonc PRIVATE ${peglib_SOURCE_DIR}
                                          ${toml_SOURCE_DIR}/include
                                          ${semver_SOURCE_DIR}/include
                                          ${fast_float_SOURCE_DIR}/include)
target_sources(codonc PRIVATE ${CODON_CPPFILES} codon_rules.cpp omp_rules.cpp)
if(ASAN)
  target_compile_options(
    codonc PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address"
                   "-fsanitize-recover=address")
  target_link_libraries(
    codonc PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address"
                   "-fsanitize-recover=address")
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
  set_source_files_properties(codon_rules.cpp codon/parser/peg/peg.cpp
                              PROPERTIES COMPILE_FLAGS "-O2")
endif()
llvm_map_components_to_libnames(
  LLVM_LIBS
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
  target_link_libraries(codonc PRIVATE ${LLVM_LIBS} fmt dl codonrt)
else()
  target_link_libraries(codonc PRIVATE ${STATIC_LIBCPP} ${LLVM_LIBS} fmt dl codonrt)
endif()

# Gather headers
add_custom_target(
  headers ALL
  COMMENT "Collecting headers"
  BYPRODUCTS "${CMAKE_BINARY_DIR}/include"
  VERBATIM
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/include/codon"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/codon"
          "${CMAKE_BINARY_DIR}/include/codon"
  COMMAND find "${CMAKE_BINARY_DIR}/include" -type f ! -name "*.h" -exec rm {}
          \\;)
add_dependencies(headers codonrt codonc)

# Prepare lib directory for plugin compilation
add_custom_target(
  libs ALL
  COMMENT "Collecting libraries"
  BYPRODUCTS "${CMAKE_BINARY_DIR}/lib"
  VERBATIM
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/lib/codon"
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/libcodonc${CMAKE_SHARED_LIBRARY_SUFFIX}"
    "${CMAKE_BINARY_DIR}/lib/codon"
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/libcodonrt${CMAKE_SHARED_LIBRARY_SUFFIX}"
    "${CMAKE_BINARY_DIR}/lib/codon"
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/libomp${CMAKE_SHARED_LIBRARY_SUFFIX}"
    "${CMAKE_BINARY_DIR}/lib/codon")
add_dependencies(libs codonrt codonc)

# Codon command-line tool
add_executable(codon codon/app/main.cpp)
target_link_libraries(codon PUBLIC ${STATIC_LIBCPP} fmt codonc codon_jupyter Threads::Threads)

# Codon test Download and unpack googletest at configure time
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
option(INSTALL_GTEST "Enable installation of googletest." OFF)
FetchContent_MakeAvailable(googletest)
enable_testing()
set(CODON_TEST_CPPFILES
    test/main.cpp
    test/cir/analyze/dominator.cpp
    test/cir/analyze/reaching.cpp
    test/cir/base.cpp
    test/cir/constant.cpp
    test/cir/flow.cpp
    test/cir/func.cpp
    test/cir/instr.cpp
    test/cir/module.cpp
    test/cir/transform/manager.cpp
    test/cir/types/types.cpp
    test/cir/util/matching.cpp
    test/cir/value.cpp
    test/cir/var.cpp
    test/types.cpp)
add_executable(codon_test ${CODON_TEST_CPPFILES})
target_include_directories(codon_test PRIVATE test/cir
                                              "${gc_SOURCE_DIR}/include")
target_link_libraries(codon_test fmt codonc codonrt gtest_main)
target_compile_definitions(codon_test
                           PRIVATE TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test")

install(TARGETS codonrt codonc codon_jupyter DESTINATION lib/codon)
install(FILES ${CMAKE_BINARY_DIR}/libomp${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION lib/codon)
install(TARGETS codon DESTINATION bin)
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/codon DESTINATION include)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/stdlib DESTINATION lib/codon)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/jit/ DESTINATION python)
install(DIRECTORY DESTINATION lib/codon/plugins)
