set(CPM_DOWNLOAD_VERSION 0.32.3)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake...")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()
include(${CPM_DOWNLOAD_LOCATION})

# CPMAddPackage(
#     NAME  libunistd
#     GITHUB_REPOSITORY "robinrowe/libunistd"
#     VERSION master
#     GIT_TAG ff791340466556f815e04a0280c31ad266a1d15d
#     EXCLUDE_FROM_ALL YES
#     OPTIONS "CMAKE_BUILD_TYPE Release")
# if(libunistd_ADDED)
#     set_target_properties(libportable PROPERTIES CMAKE_CXX_STANDARD 17)
#     set_target_properties(libsqlite PROPERTIES EXCLUDE_FROM_ALL ON)
#     set_target_properties(libuuid PROPERTIES EXCLUDE_FROM_ALL ON)
#     set_target_properties(libregex PROPERTIES EXCLUDE_FROM_ALL ON)
#     set_target_properties(libxxhash PROPERTIES EXCLUDE_FROM_ALL ON)
# endif()

CPMAddPackage(
    NAME peglib
    GITHUB_REPOSITORY "exaloop/cpp-peglib"
    GIT_TAG codon
    OPTIONS "BUILD_TESTS OFF")

CPMAddPackage(
    NAME fmt
    GITHUB_REPOSITORY "fmtlib/fmt"
    GIT_TAG 9.1.0
    OPTIONS "CMAKE_POSITION_INDEPENDENT_CODE ON"
            "FMT_INSTALL ON")

CPMAddPackage(
    NAME toml
    GITHUB_REPOSITORY "marzer/tomlplusplus"
    GIT_TAG v3.2.0)

CPMAddPackage(
    NAME semver
    GITHUB_REPOSITORY "Neargye/semver"
    GIT_TAG v0.3.0)

CPMAddPackage(
    NAME zlibng
    GITHUB_REPOSITORY "zlib-ng/zlib-ng"
    VERSION 2.1.2
    GIT_TAG 2.1.2
    EXCLUDE_FROM_ALL YES
    OPTIONS "HAVE_OFF64_T ON"
            "ZLIB_COMPAT ON"
            "WITH_GTEST OFF"
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
    EXCLUDE_FROM_ALL YES
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

if (WIN32)
    CPMAddPackage(
        NAME bdwgc
        GITHUB_REPOSITORY "ivmai/bdwgc"
        VERSION 8.0.5
        GIT_TAG d0ba209660ea8c663e06d9a68332ba5f42da54ba
        EXCLUDE_FROM_ALL YES
        OPTIONS "CMAKE_POSITION_INDEPENDENT_CODE ON"
                "BUILD_SHARED_LIBS OFF"
                "enable_threads ON"
                "enable_large_config ON"
                "enable_thread_local_alloc ON"
                "disable_handle_fork ON"
                "enable_single_obj_compilation on")
else()
    CPMAddPackage(
        NAME bdwgc
        GITHUB_REPOSITORY "ivmai/bdwgc"
        VERSION 8.0.5
        GIT_TAG d0ba209660ea8c663e06d9a68332ba5f42da54ba
        EXCLUDE_FROM_ALL YES
        OPTIONS "CMAKE_POSITION_INDEPENDENT_CODE ON"
                "BUILD_SHARED_LIBS OFF"
                "enable_threads ON"
                "enable_large_config ON"
                "enable_thread_local_alloc ON"
                "enable_handle_fork ON")
endif()
if(bdwgc_ADDED)
    set_target_properties(cord PROPERTIES EXCLUDE_FROM_ALL ON)
    # if(WIN32 AND NOT EXISTS "${bdwgc_SOURCE_DIR}/libatomic_ops")
    #     file(COPY "${libatomic_ops_SOURCE_DIR}" DESTINATION "${bdwgc_SOURCE_DIR}/")
    #     file(RENAME "${bdwgc_SOURCE_DIR}/libatomic_ops-src" "${bdwgc_SOURCE_DIR}/libatomic_ops")
    # endif()
endif()

CPMAddPackage(
    NAME openmp
    GITHUB_REPOSITORY "exaloop/openmp"
    GIT_TAG 376ec88480b9eeead8193a6bd4bb743efc6c5ea5
    EXCLUDE_FROM_ALL YES
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
        "${backtrace_SOURCE_DIR}/dwarf.c"
        "${backtrace_SOURCE_DIR}/fileline.c"
        "${backtrace_SOURCE_DIR}/posix.c"
        "${backtrace_SOURCE_DIR}/print.c"
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
        list(APPEND backtrace_SOURCES
            "${backtrace_SOURCE_DIR}/macho.c"
            "${backtrace_SOURCE_DIR}/mmapio.c"
            "${backtrace_SOURCE_DIR}/mmap.c"
            "${backtrace_SOURCE_DIR}/simple.c"
            "${backtrace_SOURCE_DIR}/backtrace.c")
    elseif(WIN32)
        set(HAVE_SYS_MMAN_H 0)
        list(APPEND backtrace_SOURCES
            "${backtrace_SOURCE_DIR}/pecoff.c"
            "${backtrace_SOURCE_DIR}/alloc.c"
            "${backtrace_SOURCE_DIR}/read.c"
            "${backtrace_SOURCE_DIR}/nounwind.c")
    else()
        set(HAVE_MACH_O_DYLD_H 0)
        list(APPEND backtrace_SOURCES
            "${backtrace_SOURCE_DIR}/elf.c"
            "${backtrace_SOURCE_DIR}/mmapio.c"
            "${backtrace_SOURCE_DIR}/mmap.c"
            "${backtrace_SOURCE_DIR}/simple.c"
            "${backtrace_SOURCE_DIR}/backtrace.c")
    endif()
    # Generate backtrace-supported.h based on the above.
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/backtrace-supported.h.in
        ${backtrace_SOURCE_DIR}/backtrace-supported.h)
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/backtrace-config.h.in
        ${backtrace_SOURCE_DIR}/config.h)
    add_library(backtrace OBJECT ${backtrace_SOURCES})
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
    EXCLUDE_FROM_ALL YES
    OPTIONS "CMAKE_POSITION_INDEPENDENT_CODE ON"
            "BUILD_SHARED_LIBS OFF"
            "RE2_BUILD_TESTING OFF")

if(APPLE AND APPLE_ARM)
    enable_language(ASM)
    CPMAddPackage(
        NAME unwind
        GITHUB_REPOSITORY "exaloop/libunwind"
        GIT_TAG e50988ccea5492b62e014408796002306b36eb9c
        OPTIONS "CMAKE_BUILD_TYPE Release"
                "LIBUNWIND_ENABLE_STATIC OFF"
                "LIBUNWIND_ENABLE_SHARED ON"
                "LIBUNWIND_INCLUDE_DOCS OFF")
endif()
