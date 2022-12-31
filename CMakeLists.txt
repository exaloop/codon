cmake_minimum_required(VERSION 3.14)
project(
  Codon
  VERSION "0.15.3"
  HOMEPAGE_URL "https://github.com/exaloop/codon"
  DESCRIPTION "high-performance, extensible Python compiler")
set(CODON_JIT_PYTHON_VERSION "0.1.1")
configure_file("${PROJECT_SOURCE_DIR}/cmake/config.h.in"
               "${PROJECT_SOURCE_DIR}/codon/config/config.h")
configure_file("${PROJECT_SOURCE_DIR}/cmake/config.py.in"
               "${PROJECT_SOURCE_DIR}/extra/python/codon/version.py")

option(CODON_JUPYTER "build Codon Jupyter server" OFF)
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

# Codon runtime library
set(CODONRT_FILES codon/runtime/lib.h codon/runtime/lib.cpp
                  codon/runtime/re.cpp codon/runtime/exc.cpp
                  codon/runtime/gpu.cpp)
add_library(codonrt SHARED ${CODONRT_FILES})
add_dependencies(codonrt zlibstatic gc backtrace bz2 liblzma re2)
if(APPLE AND APPLE_ARM)
  add_dependencies(codonrt unwind_shared)
endif()
target_include_directories(codonrt PRIVATE ${backtrace_SOURCE_DIR}
                                           ${re2_SOURCE_DIR}
                                           "${gc_SOURCE_DIR}/include" runtime)
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
if(APPLE AND APPLE_ARM)
  add_custom_command(
    TARGET codonrt
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:unwind_shared>
            ${CMAKE_BINARY_DIR})
endif()

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
    codon/parser/ast/types/type.h
    codon/parser/ast/types/link.h
    codon/parser/ast/types/class.h
    codon/parser/ast/types/function.h
    codon/parser/ast/types/union.h
    codon/parser/ast/types/static.h
    codon/parser/ast/types/traits.h
    codon/parser/cache.h
    codon/parser/common.h
    codon/parser/ctx.h
    codon/parser/peg/peg.h
    codon/parser/peg/rules.h
    codon/parser/visitors/doc/doc.h
    codon/parser/visitors/format/format.h
    codon/parser/visitors/simplify/simplify.h
    codon/parser/visitors/simplify/ctx.h
    codon/parser/visitors/translate/translate.h
    codon/parser/visitors/translate/translate_ctx.h
    codon/parser/visitors/typecheck/typecheck.h
    codon/parser/visitors/typecheck/ctx.h
    codon/parser/visitors/visitor.h
    codon/sir/analyze/analysis.h
    codon/sir/analyze/dataflow/capture.h
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
    codon/sir/llvm/gpu.h
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
    codon/sir/transform/pythonic/list.h
    codon/sir/transform/pythonic/str.h
    codon/sir/transform/rewrite.h
    codon/sir/types/types.h
    codon/sir/util/cloning.h
    codon/sir/util/context.h
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
    extra/jupyter/jupyter.h
    codon/compiler/jit_extern.h)
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
    codon/sir/attribute.cpp
    codon/sir/analyze/analysis.cpp
    codon/sir/analyze/dataflow/capture.cpp
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
    codon/sir/llvm/gpu.cpp
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
    codon/sir/transform/pythonic/list.cpp
    codon/sir/transform/pythonic/str.cpp
    codon/sir/types/types.cpp
    codon/sir/util/cloning.cpp
    codon/sir/util/format.cpp
    codon/sir/util/inlining.cpp
    codon/sir/util/irtools.cpp
    codon/sir/util/matching.cpp
    codon/sir/util/outlining.cpp
    codon/sir/util/side_effect.cpp
    codon/sir/util/visitor.cpp
    codon/sir/value.cpp
    codon/sir/var.cpp
    codon/util/common.cpp
    extra/jupyter/jupyter.cpp)
add_library(codonc SHARED ${CODON_HPPFILES})
target_include_directories(codonc PRIVATE ${peglib_SOURCE_DIR} ${toml_SOURCE_DIR}/include ${semver_SOURCE_DIR}/include)
target_sources(codonc PRIVATE ${CODON_CPPFILES} codon_rules.cpp omp_rules.cpp)
if(CODON_JUPYTER)
  add_compile_definitions(CODON_JUPYTER)
  add_dependencies(codonc xeus-static nlohmann_json)
  target_link_libraries(codonc PRIVATE xeus-static)
endif()
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

if(APPLE AND APPLE_ARM)
  # add_custom_target(
  #   libs_apple_arm ALL
  #   COMMENT "Collecting Apple-specific libraries"
  #   BYPRODUCTS "${CMAKE_BINARY_DIR}/lib"
  #   VERBATIM
  #   COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/lib/codon"
  #   COMMAND
  #     ${CMAKE_COMMAND} -E copy
  #     "${CMAKE_BINARY_DIR}/libunwind${CMAKE_SHARED_LIBRARY_SUFFIX}"
  #     "${CMAKE_BINARY_DIR}/lib/codon")
  # add_dependencies(libs_apple_arm codonrt)
endif()

# Codon command-line tool
add_executable(codon codon/app/main.cpp)
target_link_libraries(codon PUBLIC ${STATIC_LIBCPP} fmt codonc Threads::Threads)

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
target_include_directories(codon_test PRIVATE test/sir
                                              "${gc_SOURCE_DIR}/include")
target_link_libraries(codon_test fmt codonc codonrt gtest_main)
target_compile_definitions(codon_test
                           PRIVATE TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test")

install(TARGETS codonrt codonc DESTINATION lib/codon)
install(FILES ${CMAKE_BINARY_DIR}/libomp${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION lib/codon)
install(TARGETS codon DESTINATION bin)
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/codon DESTINATION include)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/stdlib DESTINATION lib/codon)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/extra/python/ DESTINATION python)
install(DIRECTORY DESTINATION lib/codon/plugins)
