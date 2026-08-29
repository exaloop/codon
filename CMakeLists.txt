cmake_minimum_required(VERSION 3.14)
project(
  Codon
  VERSION "0.19.6"
  HOMEPAGE_URL "https://github.com/exaloop/codon"
  DESCRIPTION "high-performance, extensible Python compiler")
set(CODON_JIT_PYTHON_VERSION "0.4.6")
configure_file("${PROJECT_SOURCE_DIR}/cmake/config.h.in"
               "${PROJECT_SOURCE_DIR}/codon/config/config.h")
configure_file("${PROJECT_SOURCE_DIR}/cmake/config.py.in"
               "${PROJECT_SOURCE_DIR}/jit/codon/version.py")

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
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
if(WIN32)
  # clang must use the DLL MSVC runtime; CMake's clang-GNU link rule otherwise omits
  # the CRT import libs, so link them explicitly. GC_NOT_DLL: bdwgc linked statically.
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fms-runtime-lib=dll")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-runtime-lib=dll")
  add_compile_definitions(GC_NOT_DLL)
  set(_codon_win_crt "-lmsvcrt -lvcruntime -lucrt -lmsvcprt -loldnames -llegacy_stdio_definitions")
  set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} ${_codon_win_crt}")
  set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} ${_codon_win_crt}")
endif()
include_directories(.)

set(APPLE_ARM OFF)
if (APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
  set(APPLE_ARM ON)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(LLVM REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
if(WIN32)
  # GC-patched libomp built from the exaloop LLVM fork's openmp/ with clang-cl and
  # installed alongside LLVM (lib/libomp.lib, bin/libomp.dll). This imported `omp`
  # target mirrors the one the prebuilt LLVM provides on Linux/macOS.
  add_library(omp SHARED IMPORTED)
  set_target_properties(omp PROPERTIES
    IMPORTED_IMPLIB "${LLVM_LIBRARY_DIR}/libomp.lib"
    IMPORTED_LOCATION "${LLVM_TOOLS_BINARY_DIR}/libomp.dll")
endif()
include(${CMAKE_SOURCE_DIR}/cmake/deps.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CMakeRC.cmake)

set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
if(APPLE)
  set(CMAKE_INSTALL_RPATH "@loader_path;@loader_path/../lib/codon")
else()
  set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib/codon")
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
if(WIN32)
  set_target_properties(codon_jupyter PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Codon runtime library
set(CODONRT_FILES codon/runtime/lib.h codon/runtime/lib.cpp
                  codon/runtime/re.cpp codon/runtime/exc.cpp
                  codon/runtime/numpy/sort.cpp
                  codon/runtime/numpy/loops.cpp codon/runtime/numpy/zmath.cpp)
add_library(codonrt SHARED ${CODONRT_FILES})
if(WIN32)
  # MSVC's CRT lacks the compiler-rt builtins clang emits for float16/bf16 and 128-bit
  # ints (Int[128]/UInt[128] div/mod, int<->fp). On Linux/macOS these come from
  # libgcc/compiler-rt; on Windows we vendor them. They must be codonrt's OWN sources
  # (NOT whole-archived codonfloat, whose symbols WINDOWS_EXPORT_ALL_SYMBOLS does not
  # export) so the JIT resolves them via the process symbol table and AOT via codonrt's
  # import lib. So on Windows codonrt compiles floatlib directly and does not link
  # codonfloat (clang supports __int128 even on windows-msvc).
  target_sources(codonrt PRIVATE
                 # float16/bf16 conversions (same set as codonfloat)
                 codon/runtime/floatlib/extenddftf2.c codon/runtime/floatlib/truncdfhf2.c
                 codon/runtime/floatlib/extendhfsf2.c codon/runtime/floatlib/truncdfsf2.c
                 codon/runtime/floatlib/extendhftf2.c codon/runtime/floatlib/truncsfbf2.c
                 codon/runtime/floatlib/extendsfdf2.c codon/runtime/floatlib/truncsfhf2.c
                 codon/runtime/floatlib/extendsftf2.c codon/runtime/floatlib/trunctfdf2.c
                 codon/runtime/floatlib/trunctfhf2.c codon/runtime/floatlib/truncdfbf2.c
                 codon/runtime/floatlib/trunctfsf2.c
                 # 128-bit int div/mod and int<->fp conversions
                 codon/runtime/floatlib/divti3.c codon/runtime/floatlib/modti3.c
                 codon/runtime/floatlib/udivti3.c codon/runtime/floatlib/umodti3.c
                 codon/runtime/floatlib/udivmodti4.c codon/runtime/floatlib/clzti2.c
                 codon/runtime/floatlib/floattidf.c codon/runtime/floatlib/floatuntidf.c
                 codon/runtime/floatlib/floattisf.c codon/runtime/floatlib/floatuntisf.c
                 codon/runtime/floatlib/fixdfti.c codon/runtime/floatlib/fixunsdfti.c)
  set_source_files_properties(
    codon/runtime/floatlib/extenddftf2.c codon/runtime/floatlib/truncdfhf2.c
    codon/runtime/floatlib/extendhfsf2.c codon/runtime/floatlib/truncdfsf2.c
    codon/runtime/floatlib/extendhftf2.c codon/runtime/floatlib/truncsfbf2.c
    codon/runtime/floatlib/extendsfdf2.c codon/runtime/floatlib/truncsfhf2.c
    codon/runtime/floatlib/extendsftf2.c codon/runtime/floatlib/trunctfdf2.c
    codon/runtime/floatlib/trunctfhf2.c codon/runtime/floatlib/truncdfbf2.c
    codon/runtime/floatlib/trunctfsf2.c
    PROPERTIES COMPILE_DEFINITIONS COMPILER_RT_HAS_FLOAT16)
  set_target_properties(codonrt PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
  add_dependencies(codonrt zlibstatic gc bz2 liblzma re2 hwy hwy_contrib openblas)
else()
  add_dependencies(codonrt zlibstatic gc backtrace bz2 liblzma
                           re2 hwy hwy_contrib fast_float)
endif()

if(WIN32)
  # system libraries (libgfortran/libquadmath/libgcc) deferred on Windows (numpy/BLAS off)
elseif(DEFINED ENV{CODON_SYSTEM_LIBRARIES})
  if(APPLE)
    set(copied_libgfortran "${CMAKE_BINARY_DIR}/libgfortran.5${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(copied_libquadmath "${CMAKE_BINARY_DIR}/libquadmath.0${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(copied_libgcc      "${CMAKE_BINARY_DIR}/libgcc_s.1.1${CMAKE_SHARED_LIBRARY_SUFFIX}")
  else()
    set(copied_libgfortran "${CMAKE_BINARY_DIR}/libgfortran${CMAKE_SHARED_LIBRARY_SUFFIX}.5")
    set(copied_libquadmath "${CMAKE_BINARY_DIR}/libquadmath${CMAKE_SHARED_LIBRARY_SUFFIX}.0")
    set(copied_libgcc      "${CMAKE_BINARY_DIR}/libgcc_s${CMAKE_SHARED_LIBRARY_SUFFIX}.1")
  endif()

  add_custom_command(
    OUTPUT ${copied_libgfortran}
    DEPENDS "${CMAKE_SOURCE_DIR}/scripts/get_system_libs.sh"
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/get_system_libs.sh "$ENV{CODON_SYSTEM_LIBRARIES}" ${CMAKE_BINARY_DIR}
    COMMENT "Copying system libraries to build directory")

  add_custom_target(copy_libraries ALL DEPENDS ${copied_libgfortran})
  add_dependencies(codonrt copy_libraries)

  add_library(libgfortran SHARED IMPORTED)
  set_target_properties(libgfortran PROPERTIES IMPORTED_LOCATION ${copied_libgfortran})
  target_link_libraries(codonrt PRIVATE libgfortran)
else()
  message(FATAL_ERROR "Set 'CODON_SYSTEM_LIBRARIES' to the directory containing system libraries.")
endif()

target_include_directories(codonrt PRIVATE ${backtrace_SOURCE_DIR}
                                           ${re2_SOURCE_DIR}
                                           ${highway_SOURCE_DIR}
                                           "${gc_SOURCE_DIR}/include"
                                           "${fast_float_SOURCE_DIR}/include" runtime)
if(WIN32)
  # dbghelp: native Windows backtrace (replaces libbacktrace on Windows).
  # omp: GC-patched libomp (enables @par); brings in __kmpc_set_gc_callbacks.
  target_link_libraries(codonrt PRIVATE fmt LLVMSupport dbghelp omp)
else()
  target_link_libraries(codonrt PRIVATE fmt omp backtrace LLVMSupport)
endif()

if(APPLE)
  set(COMPILER_RT_NAMES clang_rt.osx libclang_rt.osx.a)
  set(COMPILER_RT_PATHS
    "${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION_MAJOR}/lib/darwin"
    "${LLVM_LIBRARY_DIR}/clang/${LLVM_PACKAGE_VERSION}/lib/darwin"
  )
elseif(UNIX)
  set(COMPILER_RT_NAMES clang_rt.builtins libclang_rt.builtins.a)

  set(COMPILER_RT_TRIPLES
    "${CMAKE_SYSTEM_PROCESSOR}-unknown-linux-gnu"
    "${CMAKE_SYSTEM_PROCESSOR}-linux-gnu"
    "aarch64-unknown-linux-gnu"
    "x86_64-unknown-linux-gnu"
  )

  foreach(TRIPLE IN LISTS COMPILER_RT_TRIPLES)
    list(APPEND COMPILER_RT_PATHS
      "${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION_MAJOR}/lib/${TRIPLE}"
      "${LLVM_LIBRARY_DIR}/clang/${LLVM_PACKAGE_VERSION}/lib/${TRIPLE}"
    )
  endforeach()

  list(APPEND COMPILER_RT_PATHS
    "${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION_MAJOR}/lib/linux"
    "${LLVM_LIBRARY_DIR}/clang/${LLVM_PACKAGE_VERSION}/lib/linux"
  )
endif()

if(NOT WIN32)
  find_library(COMPILER_RT_LIB
    NAMES ${COMPILER_RT_NAMES}
    PATHS ${COMPILER_RT_PATHS}
    NO_DEFAULT_PATH
  )

  if(NOT COMPILER_RT_LIB)
    message(FATAL_ERROR
      "Could not find compiler-rt. Tried ${COMPILER_RT_NAMES} in ${COMPILER_RT_PATHS}")
  endif()
  message(STATUS "Found compiler-rt: ${COMPILER_RT_LIB}")
endif()
if(APPLE)
  target_link_libraries(
    codonrt
    PRIVATE -Wl,-force_load,$<TARGET_FILE:zlibstatic>
            -Wl,-force_load,$<TARGET_FILE:gc>
            -Wl,-force_load,$<TARGET_FILE:bz2>
            -Wl,-force_load,$<TARGET_FILE:liblzma>
            -Wl,-force_load,$<TARGET_FILE:re2>
            -Wl,-force_load,$<TARGET_FILE:hwy>
            -Wl,-force_load,$<TARGET_FILE:hwy_contrib>
            -Wl,-force_load,${COMPILER_RT_LIB})
  target_link_libraries(codonrt PUBLIC "-framework Accelerate")
elseif(WIN32)
  # zlibstatic linked normally (NOT whole-archived): zlib-ng embeds a Windows
  # version resource (.res) in its static lib that lld-link's /WHOLEARCHIVE rejects.
  target_link_libraries(
    codonrt
    PRIVATE $<TARGET_FILE:gc>
            $<TARGET_FILE:zlibstatic>
            -Wl,/WHOLEARCHIVE:$<TARGET_FILE:bz2>
            -Wl,/WHOLEARCHIVE:$<TARGET_FILE:liblzma>
            -Wl,/WHOLEARCHIVE:$<TARGET_FILE:re2>
            -Wl,/WHOLEARCHIVE:$<TARGET_FILE:hwy>
            -Wl,/WHOLEARCHIVE:$<TARGET_FILE:hwy_contrib>
            # OpenBLAS lives in libopenblas.dll; codon's numpy.linalg calls cblas_*/LAPACK
            # from JIT/AOT code, not from codonrt itself. Link its import lib and force a
            # reference so libopenblas.dll is a load-time dependency (and thus loaded for
            # the JIT symbol resolver).
            openblas
            -Wl,/INCLUDE:cblas_dgemm
            # zlib (gz*) and bzip2 (BZ2_*) come from static libs whose symbols
            # WINDOWS_EXPORT_ALL_SYMBOLS does NOT re-export (it only scans codonrt's own
            # objects). codon's gzip/bz2 stdlib calls them from JIT/AOT code, so the JIT's
            # process symbol resolver needs them in codonrt.dll's export table. Force a
            # reference (/INCLUDE, so the archive member is pulled in) + export each.
            -Wl,/INCLUDE:gzopen
            -Wl,/EXPORT:gzopen -Wl,/EXPORT:gzclose -Wl,/EXPORT:gzread
            -Wl,/EXPORT:gzwrite -Wl,/EXPORT:gzgetc -Wl,/EXPORT:gzgets
            -Wl,/EXPORT:gzseek -Wl,/EXPORT:gztell -Wl,/EXPORT:gzflush
            -Wl,/EXPORT:gzerror
            -Wl,/INCLUDE:BZ2_bzopen
            -Wl,/EXPORT:BZ2_bzopen -Wl,/EXPORT:BZ2_bzclose -Wl,/EXPORT:BZ2_bzread
            -Wl,/EXPORT:BZ2_bzwrite -Wl,/EXPORT:BZ2_bzflush -Wl,/EXPORT:BZ2_bzerror)
else()
  add_dependencies(codonrt openblas_64)

  # Make sure compiler-rt symbols are visible
  set(LLVM_OBJCOPY "${LLVM_BINARY_DIR}/bin/llvm-objcopy")
  if(NOT EXISTS "${LLVM_OBJCOPY}")
    find_program(LLVM_OBJCOPY
      NAMES llvm-objcopy objcopy
      REQUIRED
    )
  endif()
  set(COMPILER_RT_DEFAULT "${CMAKE_BINARY_DIR}/libclang_rt.builtins.default.a")
  set(COMPILER_RT_DEFAULT_VIS_SYMBOLS
    __truncdfhf2
    __truncdfbf2
    __truncdfsf2
    __truncsfbf2
    __truncsfhf2
    __trunctfdf2
    __trunctfbf2
    __trunctfhf2
    __trunctfsf2
    __truncxfbf2
    __trunctfxf2
    __trunchfbf2
    __truncxfhf2
    __extendbfsf2
    __extendsfdf2
    __extendhfsf2
    __extenddftf2
    __extendhftf2
    __extendsftf2
    __extendhfxf2
    __extendxftf2
  )
  set(COMPILER_RT_OBJCOPY_ARGS)
  foreach(SYM IN LISTS COMPILER_RT_DEFAULT_VIS_SYMBOLS)
    list(APPEND COMPILER_RT_OBJCOPY_ARGS
      "--set-symbol-visibility=${SYM}=default")
  endforeach()
  add_custom_command(
    OUTPUT "${COMPILER_RT_DEFAULT}"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${COMPILER_RT_LIB}"
            "${COMPILER_RT_DEFAULT}"
    COMMAND "${LLVM_OBJCOPY}"
            ${COMPILER_RT_OBJCOPY_ARGS}
            "${COMPILER_RT_DEFAULT}"
    DEPENDS "${COMPILER_RT_LIB}"
    VERBATIM
  )
  add_custom_target(compiler_rt_default_visibility
    DEPENDS "${COMPILER_RT_DEFAULT}")
  add_dependencies(codonrt compiler_rt_default_visibility)

  target_link_libraries(
    codonrt
    PRIVATE -Wl,--whole-archive $<TARGET_FILE:zlibstatic> $<TARGET_FILE:gc>
            $<TARGET_FILE:bz2> $<TARGET_FILE:liblzma> $<TARGET_FILE:re2>
            $<TARGET_FILE:openblas_64> $<TARGET_FILE:hwy> $<TARGET_FILE:hwy_contrib>
            ${COMPILER_RT_DEFAULT} -Wl,--no-whole-archive)
endif()
if(ASAN)
  target_compile_options(
    codonrt PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address"
                    "-fsanitize-recover=address")
  target_link_libraries(
    codonrt PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address"
                    "-fsanitize-recover=address")
endif()
# Copy libomp next to codonrt so JIT (codon run) and the build tree can load it.
add_custom_command(
  TARGET codonrt
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:omp>
          ${CMAKE_BINARY_DIR})

if(WIN32)
  # Same for libopenblas.dll (numpy.linalg).
  add_custom_command(
    TARGET codonrt
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:openblas>
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
  codon/compiler/jit_extern.h
  codon/compiler/memory_manager.h
  codon/compiler/options.h
  codon/dsl/dsl.h
  codon/dsl/plugins.h
  codon/parser/ast.h
  codon/parser/match.h
  codon/parser/ast/node.h
  codon/parser/ast/expr.h
  codon/parser/ast/stmt.h
  codon/parser/ast/types.h
  codon/parser/ast/attr.h
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
  codon/parser/visitors/scoping/scoping.h
  codon/parser/visitors/translate/translate.h
  codon/parser/visitors/translate/translate_ctx.h
  codon/parser/visitors/typecheck/typecheck.h
  codon/parser/visitors/typecheck/ctx.h
  codon/parser/visitors/visitor.h
  codon/cir/analyze/analysis.h
  codon/cir/analyze/dataflow/capture.h
  codon/cir/analyze/dataflow/cfg.h
  codon/cir/analyze/dataflow/dominator.h
  codon/cir/analyze/dataflow/reaching.h
  codon/cir/analyze/module/global_vars.h
  codon/cir/analyze/module/side_effect.h
  codon/cir/attribute.h
  codon/cir/base.h
  codon/cir/const.h
  codon/cir/dsl/codegen.h
  codon/cir/dsl/nodes.h
  codon/cir/flow.h
  codon/cir/func.h
  codon/cir/instr.h
  codon/cir/llvm/gpu.h
  codon/cir/llvm/llvisitor.h
  codon/cir/llvm/llvm.h
  codon/cir/llvm/optimize.h
  codon/cir/module.h
  codon/cir/pyextension.h
  codon/cir/cir.h
  codon/cir/transform/cleanup/canonical.h
  codon/cir/transform/cleanup/dead_code.h
  codon/cir/transform/cleanup/global_demote.h
  codon/cir/transform/cleanup/replacer.h
  codon/cir/transform/folding/const_fold.h
  codon/cir/transform/folding/const_prop.h
  codon/cir/transform/folding/folding.h
  codon/cir/transform/folding/rule.h
  codon/cir/transform/lowering/async_for.h
  codon/cir/transform/lowering/await.h
  codon/cir/transform/lowering/imperative.h
  codon/cir/transform/lowering/pipeline.h
  codon/cir/transform/manager.h
  codon/cir/transform/parallel/openmp.h
  codon/cir/transform/parallel/schedule.h
  codon/cir/transform/pass.h
  codon/cir/transform/pythonic/dict.h
  codon/cir/transform/pythonic/generator.h
  codon/cir/transform/pythonic/io.h
  codon/cir/transform/pythonic/list.h
  codon/cir/transform/pythonic/str.h
  codon/cir/transform/rewrite.h
  codon/cir/type.h
  codon/cir/util/cloning.h
  codon/cir/util/context.h
  codon/cir/util/format.h
  codon/cir/util/inlining.h
  codon/cir/util/irtools.h
  codon/cir/util/iterators.h
  codon/cir/util/matching.h
  codon/cir/util/operator.h
  codon/cir/util/outlining.h
  codon/cir/util/packs.h
  codon/cir/util/side_effect.h
  codon/cir/util/visitor.h
  codon/cir/value.h
  codon/cir/llvm/native/native.h
  codon/cir/llvm/native/targets/aarch64.h
  codon/cir/llvm/native/targets/arm.h
  codon/cir/llvm/native/targets/target.h
  codon/cir/llvm/native/targets/x86.h
  codon/cir/transform/numpy/numpy.h
  codon/cir/transform/numpy/indexing.h
  codon/cir/var.h
  codon/util/common.h
  codon/util/serialize.h
  codon/util/tser.h)
set(CODON_CPPFILES
  codon/compiler/compiler.cpp
  codon/compiler/debug_listener.cpp
  codon/compiler/engine.cpp
  codon/compiler/error.cpp
  codon/compiler/jit.cpp
  codon/compiler/memory_manager.cpp
  codon/compiler/options.cpp
  codon/dsl/plugins.cpp
  codon/parser/ast/expr.cpp
  codon/parser/ast/attr.cpp
  codon/parser/ast/stmt.cpp
  codon/parser/ast/types/type.cpp
  codon/parser/ast/types/link.cpp
  codon/parser/ast/types/class.cpp
  codon/parser/ast/types/function.cpp
  codon/parser/ast/types/union.cpp
  codon/parser/ast/types/static.cpp
  codon/parser/ast/types/traits.cpp
  codon/parser/cache.cpp
  codon/parser/match.cpp
  codon/parser/common.cpp
  codon/parser/peg/peg.cpp
  codon/parser/visitors/doc/doc.cpp
  codon/parser/visitors/format/format.cpp
  codon/parser/visitors/scoping/scoping.cpp
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
  codon/parser/visitors/typecheck/import.cpp
  codon/parser/visitors/typecheck/loops.cpp
  codon/parser/visitors/typecheck/op.cpp
  codon/parser/visitors/typecheck/error.cpp
  codon/parser/visitors/typecheck/special.cpp
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
  codon/cir/transform/lowering/async_for.cpp
  codon/cir/transform/lowering/await.cpp
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
  codon/cir/type.cpp
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
  codon/cir/llvm/native/native.cpp
  codon/cir/llvm/native/targets/aarch64.cpp
  codon/cir/llvm/native/targets/arm.cpp
  codon/cir/llvm/native/targets/x86.cpp
  codon/cir/transform/numpy/expr.cpp
  codon/cir/transform/numpy/forward.cpp
  codon/cir/transform/numpy/indexing.cpp
  codon/cir/transform/numpy/numpy.cpp
  codon/util/common.cpp)
add_library(codonc SHARED ${CODON_HPPFILES})
if(WIN32)
  set_target_properties(codonc PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
  # The CLI driver (codon/app/main.cpp) is compiled into codonc on Windows so its
  # option parsing runs in the same llvm::cl registry as the cl::opt globals it
  # reads (LLVM is statically linked into both codon.exe and codonc.dll, giving
  # each binary its own registry). codon.exe becomes a thin shim (see main.cpp).
  target_sources(codonc PRIVATE codon/app/main.cpp)
  target_compile_definitions(codonc PRIVATE CODON_COMPILER_BUILD CODON_CLI_IN_COMPILER)
endif()
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

file(GLOB_RECURSE CODON_STDLIB_RESOURCES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/stdlib/*.codon"
)
cmrc_add_resource_library(
  codon-stdlib
  NAMESPACE codon
  ${CODON_STDLIB_RESOURCES}
)
set_property(TARGET codon-stdlib PROPERTY POSITION_INDEPENDENT_CODE ON)

if(WIN32)
  target_link_libraries(codonc PRIVATE ${LLVM_LIBS} fmt codonrt codon-stdlib)
else()
  target_link_libraries(codonc PRIVATE ${LLVM_LIBS} fmt dl codonrt codon-stdlib)
endif()

# Gather headers
if(WIN32)
  add_custom_target(
    headers ALL
    COMMENT "Collecting headers"
    BYPRODUCTS "${CMAKE_BINARY_DIR}/include"
    VERBATIM
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/include/codon"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/codon"
            "${CMAKE_BINARY_DIR}/include/codon")
else()
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
endif()
add_dependencies(headers codonrt codonc)

# Prepare lib directory for plugin compilation
if(WIN32)
  add_custom_target(
    libs ALL
    COMMENT "Collecting libraries"
    BYPRODUCTS "${CMAKE_BINARY_DIR}/lib"
    VERBATIM
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/lib/codon"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:codonc>" "${CMAKE_BINARY_DIR}/lib/codon"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:codonrt>" "${CMAKE_BINARY_DIR}/lib/codon")
else()
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
      "${CMAKE_BINARY_DIR}/lib/codon"
    COMMAND
      ${CMAKE_COMMAND} -E copy ${copied_libgfortran} "${CMAKE_BINARY_DIR}/lib/codon"
    COMMAND
      /bin/sh -c "test -f '${copied_libquadmath}' && ${CMAKE_COMMAND} -E copy '${copied_libquadmath}' '${CMAKE_BINARY_DIR}/lib/codon' || true"
    COMMAND
      ${CMAKE_COMMAND} -E copy ${copied_libgcc} "${CMAKE_BINARY_DIR}/lib/codon")
endif()
add_dependencies(libs codonrt codonc)

# Codon command-line tool
add_executable(codon codon/app/main.cpp)
if(WIN32)
  target_link_libraries(codon PUBLIC fmt codonc codon_jupyter Threads::Threads ${LLVM_LIBS})
else()
  target_link_libraries(codon PUBLIC fmt codonc codon_jupyter Threads::Threads)
endif()
if(ASAN)
  target_compile_options(
    codon PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address"
                  "-fsanitize-recover=address")
  target_link_libraries(
    codon PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address"
                  "-fsanitize-recover=address")
endif()

# Codon test Download and unpack googletest at configure time
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
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
    test/cir/type.cpp
    test/cir/util/matching.cpp
    test/cir/value.cpp
    test/cir/var.cpp
    test/types.cpp)
add_executable(codon_test ${CODON_TEST_CPPFILES})
if(WIN32)
  # On Windows codon_test statically links the whole compiler (see below), so keep it
  # out of the default build to avoid compiling the compiler twice. Build on demand
  # with: cmake --build <dir> --target codon_test
  set_target_properties(codon_test PROPERTIES EXCLUDE_FROM_ALL ON)
endif()
target_include_directories(codon_test PRIVATE test/cir "${gc_SOURCE_DIR}/include")
if(WIN32)
  # The unit tests ODR-use the compiler's static data members (NodeId, KEY,
  # NEG_MAGIC_NAME, IdMixin::currentId, ...). On MSVC those can't be referenced across
  # the codonc.dll boundary without __declspec(dllimport) on each (235 of them, unlike
  # ELF where it's transparent). So build the compiler sources straight into the test
  # binary and link the LLVM libs directly, instead of going through codonc.dll.
  target_sources(codon_test PRIVATE ${CODON_HPPFILES} ${CODON_CPPFILES}
                                    codon_rules.cpp omp_rules.cpp)
  target_include_directories(codon_test PRIVATE ${peglib_SOURCE_DIR}
                                                ${toml_SOURCE_DIR}/include
                                                ${semver_SOURCE_DIR}/include
                                                ${fast_float_SOURCE_DIR}/include)
  target_compile_definitions(codon_test PRIVATE CODON_COMPILER_BUILD)
  target_link_libraries(codon_test fmt codonrt codon-stdlib gtest_main ${LLVM_LIBS})
else()
  target_link_libraries(codon_test PRIVATE fmt codonc codonrt gtest_main)
endif()
if(ASAN)
  target_compile_options(
    codon_test PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address"
                       "-fsanitize-recover=address")
  target_link_libraries(
    codon_test PRIVATE "-fno-omit-frame-pointer" "-fsanitize=address"
                       "-fsanitize-recover=address")
endif()
target_compile_definitions(codon_test
                           PRIVATE TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test")

install(TARGETS codonrt codonc codon_jupyter DESTINATION lib/codon)
if(WIN32)
  # libomp.dll for runtime (JIT + AOT exes), libomp.lib so AOT `codon build` of @par
  # programs can link the __kmpc_* import lib via -L../lib/codon.
  install(FILES "${LLVM_TOOLS_BINARY_DIR}/libomp.dll" DESTINATION lib/codon)
  install(FILES "${LLVM_LIBRARY_DIR}/libomp.lib" DESTINATION lib/codon)
  # OpenBLAS: libopenblas.dll for runtime (JIT + AOT), libopenblas.lib so AOT
  # `codon build` of numpy.linalg programs can link cblas_*/LAPACK via -L../lib/codon.
  install(FILES "${OPENBLAS_WIN_DIR}/bin/libopenblas.dll" DESTINATION lib/codon)
  install(FILES "${OPENBLAS_WIN_DIR}/lib/libopenblas.lib" DESTINATION lib/codon)
endif()
if(NOT WIN32)
  install(FILES ${CMAKE_BINARY_DIR}/libomp${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION lib/codon)
  install(FILES ${copied_libgfortran} DESTINATION lib/codon)
  # only install libquadmath if it exists at build time
  install(CODE "
    file(GLOB _quadmath \"${copied_libquadmath}\")
    if(EXISTS \"\${_quadmath}\")
      file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/lib/codon\" TYPE FILE FILES \"\${_quadmath}\")
    endif()
  ")
  install(FILES ${copied_libgcc} DESTINATION lib/codon)
endif()
install(TARGETS codon DESTINATION bin)
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/codon DESTINATION include)
install(DIRECTORY ${LLVM_INCLUDE_DIRS}/llvm DESTINATION include)
install(DIRECTORY ${LLVM_INCLUDE_DIRS}/llvm-c DESTINATION include)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/stdlib DESTINATION lib/codon)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/jit/ DESTINATION python)
install(DIRECTORY DESTINATION lib/codon/plugins)

install(CODE [[
  if(APPLE)
    # Compute the real install root (supports DESTDIR)
    set(_root "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}")
    message(STATUS "fix_loader_paths.sh on: ${_root}")
    execute_process(
      COMMAND /bin/bash "${CMAKE_SOURCE_DIR}/scripts/fix_loader_paths.sh" "${_root}"
      RESULT_VARIABLE rc
    )
    if(NOT rc EQUAL 0)
      message(FATAL_ERROR "fix_loader_paths.sh failed with code ${rc}")
    endif()
  endif()
]])
