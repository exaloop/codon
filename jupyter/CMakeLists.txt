cmake_minimum_required(VERSION 3.14)
project(
  CodonJupyter
  VERSION "0.1"
  HOMEPAGE_URL "https://github.com/exaloop/codon"
  DESCRIPTION "Jupyter support for Codon")

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

if(NOT CODON_PATH)
    set(CODON_PATH "$ENV{HOME}/.codon")
endif()
message(STATUS "Found Codon in ${CODON_PATH}")
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CODON_PATH}/lib/codon/" CACHE PATH "Use the existing Codon installation" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)


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
include_directories(.)

if(WIN32)
  # Codon/LLVM (and the vcpkg x64-windows-static-md OpenSSL/sodium) use the dynamic CRT
  # (/MD). Force every dependency built here (xeus, xeus-zmq, libzmq, fmt) to match via
  # clang's -fms-runtime-lib=dll, else lld-link rejects MT(static)/MD(dynamic) mismatches.
  add_compile_options(-fms-runtime-lib=dll)
  # libzmq is linked statically here; ZMQ_STATIC stops zmq.h from marking symbols
  # __declspec(dllimport), which would otherwise need a (nonexistent) import library.
  add_compile_definitions(ZMQ_STATIC)
endif()

find_package(LLVM REQUIRED CONFIG)
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

set(CPM_DOWNLOAD_VERSION 0.32.3)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake...")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()
include(${CPM_DOWNLOAD_LOCATION})
CPMAddPackage(
    NAME xtl
    GITHUB_REPOSITORY "xtensor-stack/xtl"
    VERSION 0.7.5
    GIT_TAG 0.7.5
    EXCLUDE_FROM_ALL YES
    OPTIONS "BUILD_TESTS OFF")
CPMAddPackage(
    NAME json
    GITHUB_REPOSITORY "nlohmann/json"
    VERSION 3.11.2)
CPMAddPackage(
    NAME xeus
    GITHUB_REPOSITORY "jupyter-xeus/xeus"
    VERSION 3.0.5
    GIT_TAG 3.0.5
    EXCLUDE_FROM_ALL YES
    # xeus.patch: upstream Codon fixes (Linux libuuid). xeus-win.patch: use the Win32
    # GUID backend (ole32/rpcrt4) instead of libuuid for the clang-on-Windows build.
    PATCH_COMMAND git apply --reject --whitespace=fix ${CMAKE_SOURCE_DIR}/xeus.patch ${CMAKE_SOURCE_DIR}/xeus-win.patch
    OPTIONS "BUILD_EXAMPLES OFF"
            "XEUS_BUILD_SHARED_LIBS OFF"
            "XEUS_STATIC_DEPENDENCIES ON"
            "CMAKE_POSITION_INDEPENDENT_CODE ON"
            "XEUS_DISABLE_ARCH_NATIVE ON"
            "XEUS_USE_DYNAMIC_UUID ${XEUS_USE_DYNAMIC_UUID}")
if (xeus_ADDED)
    install(TARGETS nlohmann_json EXPORT xeus-targets)
endif()

CPMAddPackage(
  NAME libzmq
  VERSION 4.3.4
  URL https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz
  EXCLUDE_FROM_ALL YES
  OPTIONS "WITH_PERF_TOOL OFF"
          "ZMQ_BUILD_TESTS OFF"
          "ENABLE_CPACK OFF"
          "BUILD_SHARED ON"
          "WITH_LIBSODIUM OFF"
          "WITH_TLS OFF"
          "WITH_DOC OFF")
CPMAddPackage(
    NAME cppzmq
    URL https://github.com/zeromq/cppzmq/archive/refs/tags/v4.8.1.tar.gz
    VERSION 4.9.0
    EXCLUDE_FROM_ALL YES
    OPTIONS "CPPZMQ_BUILD_TESTS OFF")
CPMAddPackage(
    NAME xeus-zmq
    GITHUB_REPOSITORY "jupyter-xeus/xeus-zmq"
    VERSION 1.0.3
    GIT_TAG 1.0.3
    EXCLUDE_FROM_ALL YES
    # xeus-zmq.patch: skip the libsodium requirement on Windows — our libzmq is built
    # WITH_LIBSODIUM OFF, so the (NOT MSVC) static-deps workaround pulls in sodium for
    # nothing. (Harmless no-op on Linux/macOS where the guard already excludes them.)
    PATCH_COMMAND git apply --reject --whitespace=fix ${CMAKE_SOURCE_DIR}/xeus-zmq.patch
    OPTIONS "XEUS_ZMQ_BUILD_TESTS OFF"
            "XEUS_ZMQ_BUILD_SHARED_LIBS OFF"
            "XEUS_ZMQ_STATIC_DEPENDENCIES ON"
            "CMAKE_POSITION_INDEPENDENT_CODE ON")

# Codon Jupyter library
set(CODON_JUPYTER_FILES jupyter.h jupyter.cpp)
add_library(codon_jupyter SHARED ${CODON_JUPYTER_FILES})
if(WIN32)
  # Export startJupyterKernel (and friends) so codon.exe can resolve them from the
  # plugin DLL, mirroring the stub codon_jupyter target in the main build.
  set_target_properties(codon_jupyter PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()
target_include_directories(codon_jupyter PRIVATE "${CODON_PATH}/include" ${LLVM_INCLUDE_DIRS})
add_dependencies(codon_jupyter xeus-static xeus-zmq-static nlohmann_json)
target_link_directories(codon_jupyter PRIVATE "${CODON_PATH}/lib/codon")
target_link_libraries(codon_jupyter PRIVATE xeus-static xeus-zmq-static codonc)

if(WIN32)
  # codonc.dll links LLVM + fmt PRIVATE/statically and does not re-export them (unlike
  # ELF on Linux), so the plugin — which uses LLVM (handleAllErrors/raw_ostream) and fmt
  # via Codon's headers — must link them itself. Matches fmt 11.1.0 used by the core.
  CPMAddPackage(
      NAME fmt
      GITHUB_REPOSITORY "fmtlib/fmt"
      GIT_TAG 11.1.0
      EXCLUDE_FROM_ALL YES
      OPTIONS "CMAKE_POSITION_INDEPENDENT_CODE ON")
  llvm_map_components_to_libnames(JUPYTER_LLVM_LIBS
    AllTargetsAsmParsers AllTargetsCodeGens AllTargetsDescs AllTargetsInfos
    AggressiveInstCombine Analysis AsmParser BitWriter CodeGen Core Extensions
    IPO IRReader InstCombine Instrumentation MC MCJIT ObjCARCOpts OrcJIT Remarks
    ScalarOpts Support Symbolize Target TransformUtils Vectorize Passes)
  target_link_libraries(codon_jupyter PRIVATE fmt::fmt ${JUPYTER_LLVM_LIBS})
endif()

install(TARGETS codon_jupyter DESTINATION .)
