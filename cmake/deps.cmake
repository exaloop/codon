set(CPM_DOWNLOAD_VERSION 0.32.3)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake...")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()
include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage(
    NAME zlibng
    GITHUB_REPOSITORY "zlib-ng/zlib-ng"
    VERSION 2.0.5
    GIT_TAG 2.0.5
    OPTIONS "HAVE_OFF64_T ON"
            "ZLIB_COMPAT ON"
            "ZLIB_ENABLE_TESTS OFF"
            "CMAKE_POSITION_INDEPENDENT_CODE ON")
if(zlibng_ADDED)
    set_target_properties(zlib PROPERTIES EXCLUDE_FROM_ALL ON)
endif()

CPMAddPackage(
    NAME xz
    GITHUB_REPOSITORY "xz-mirror/xz"
    VERSION 5.2.5
    GIT_TAG e7da44d5151e21f153925781ad29334ae0786101
    OPTIONS "BUILD_SHARED_LIBS OFF"
            "CMAKE_POSITION_INDEPENDENT_CODE ON")
if(xz_ADDED)
    set_target_properties(xz PROPERTIES EXCLUDE_FROM_ALL ON)
    set_target_properties(xzdec PROPERTIES EXCLUDE_FROM_ALL ON)
endif()

CPMAddPackage(
    NAME bz2
    URL "https://www.sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz"
    DOWNLOAD_ONLY YES)
if(bz2_ADDED)
    add_library(bz2 STATIC
        "${bz2_SOURCE_DIR}/blocksort.c"
        "${bz2_SOURCE_DIR}/huffman.c"
        "${bz2_SOURCE_DIR}/crctable.c"
        "${bz2_SOURCE_DIR}/randtable.c"
        "${bz2_SOURCE_DIR}/compress.c"
        "${bz2_SOURCE_DIR}/decompress.c"
        "${bz2_SOURCE_DIR}/bzlib.c"
        "${bz2_SOURCE_DIR}/libbz2.def")
    set_target_properties(bz2 PROPERTIES
        COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64"
        POSITION_INDEPENDENT_CODE ON)
endif()

CPMAddPackage(
    NAME bdwgc
    GITHUB_REPOSITORY "ivmai/bdwgc"
    VERSION 8.0.5
    GIT_TAG d0ba209660ea8c663e06d9a68332ba5f42da54ba
    OPTIONS "CMAKE_POSITION_INDEPENDENT_CODE ON"
            "BUILD_SHARED_LIBS OFF"
            "enable_threads ON"
            "enable_large_config ON"
            "enable_thread_local_alloc ON"
            "enable_handle_fork ON")
if(bdwgc_ADDED)
    set_target_properties(cord PROPERTIES EXCLUDE_FROM_ALL ON)
endif()

CPMAddPackage(
    NAME openmp
    GITHUB_REPOSITORY "exaloop/openmp"
    GIT_TAG bdbd70a3acb5072f15c733d603b5f2b26b953555
    OPTIONS "CMAKE_BUILD_TYPE Release"
            "OPENMP_ENABLE_LIBOMPTARGET OFF"
            "OPENMP_STANDALONE_BUILD ON")

CPMAddPackage(
    NAME backtrace
    GITHUB_REPOSITORY "ianlancetaylor/libbacktrace"
    GIT_TAG d0f5e95a87a4d3e0a1ed6c069b5dae7cbab3ed2a
    DOWNLOAD_ONLY YES)
if(backtrace_ADDED)
    set(backtrace_SOURCES
        "${backtrace_SOURCE_DIR}/atomic.c"
        "${backtrace_SOURCE_DIR}/backtrace.c"
        "${backtrace_SOURCE_DIR}/dwarf.c"
        "${backtrace_SOURCE_DIR}/fileline.c"
        "${backtrace_SOURCE_DIR}/mmapio.c"
        "${backtrace_SOURCE_DIR}/mmap.c"
        "${backtrace_SOURCE_DIR}/posix.c"
        "${backtrace_SOURCE_DIR}/print.c"
        "${backtrace_SOURCE_DIR}/simple.c"
        "${backtrace_SOURCE_DIR}/sort.c"
        "${backtrace_SOURCE_DIR}/state.c")

    # https://go.googlesource.com/gollvm/+/refs/heads/master/cmake/modules/LibbacktraceUtils.cmake
    set(BACKTRACE_SUPPORTED 1)
    set(BACKTRACE_ELF_SIZE 64)
    set(HAVE_GETIPINFO 1)
    set(BACKTRACE_USES_MALLOC 1)
    set(BACKTRACE_SUPPORTS_THREADS 1)
    set(BACKTRACE_SUPPORTS_DATA 1)
    set(HAVE_SYNC_FUNCTIONS 1)
    if(APPLE)
        set(HAVE_MACH_O_DYLD_H 1)
        list(APPEND backtrace_SOURCES "${backtrace_SOURCE_DIR}/macho.c")
    else()
        set(HAVE_MACH_O_DYLD_H 0)
        list(APPEND backtrace_SOURCES "${backtrace_SOURCE_DIR}/elf.c")
    endif()
    # Generate backtrace-supported.h based on the above.
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/backtrace-supported.h.in
        ${backtrace_SOURCE_DIR}/backtrace-supported.h)
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/backtrace-config.h.in
        ${backtrace_SOURCE_DIR}/config.h)
    add_library(backtrace STATIC ${backtrace_SOURCES})
    target_include_directories(backtrace BEFORE PRIVATE "${backtrace_SOURCE_DIR}")
    set_target_properties(backtrace PROPERTIES
        COMPILE_FLAGS "-funwind-tables -D_GNU_SOURCE"
        POSITION_INDEPENDENT_CODE ON)
endif()

CPMAddPackage(
    NAME re2
    GITHUB_REPOSITORY "google/re2"
    VERSION 2022-06-01
    GIT_TAG 5723bb8950318135ed9cf4fc76bed988a087f536
    OPTIONS "CMAKE_POSITION_INDEPENDENT_CODE ON"
            "BUILD_SHARED_LIBS OFF"
            "RE2_BUILD_TESTING OFF")

if(CODON_JUPYTER)
    CPMAddPackage(
        NAME libzmq
        VERSION 4.3.4
        URL https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz
        OPTIONS "WITH_PERF_TOOL OFF"
                "ZMQ_BUILD_TESTS OFF"
                "ENABLE_CPACK OFF"
                "BUILD_SHARED ON"
                "WITH_LIBSODIUM OFF")
    CPMAddPackage(
        NAME cppzmq
        URL https://github.com/zeromq/cppzmq/archive/refs/tags/v4.8.1.tar.gz
        VERSION 4.8.1
        OPTIONS "CPPZMQ_BUILD_TESTS OFF")
    CPMAddPackage(
        NAME xtl
        GITHUB_REPOSITORY "xtensor-stack/xtl"
        VERSION 0.7.3
        GIT_TAG 0.7.3
        OPTIONS "BUILD_TESTS OFF")
    CPMAddPackage(
        NAME json
        GITHUB_REPOSITORY "nlohmann/json"
        VERSION 3.10.1)
    CPMAddPackage(
        NAME xeus
        GITHUB_REPOSITORY "jupyter-xeus/xeus"
        VERSION 2.2.0
        GIT_TAG 2.2.0
        PATCH_COMMAND sed -ibak "s/-Wunused-parameter -Wextra -Wreorder//g" CMakeLists.txt
        OPTIONS "BUILD_EXAMPLES OFF"
                "XEUS_BUILD_SHARED_LIBS OFF"
                "XEUS_STATIC_DEPENDENCIES ON"
                "CMAKE_POSITION_INDEPENDENT_CODE ON"
                "XEUS_DISABLE_ARCH_NATIVE ON")
    if (xeus_ADDED)
        install(TARGETS nlohmann_json EXPORT xeus-targets)
    endif()
endif()
