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
set_target_properties(zlib PROPERTIES EXCLUDE_FROM_ALL ON)
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
set_target_properties(cord PROPERTIES EXCLUDE_FROM_ALL ON)
CPMAddPackage(
    NAME openmp
    GITHUB_REPOSITORY "llvm-mirror/openmp"
    VERSION 9.0
    GIT_TAG release_90
    OPTIONS "OPENMP_ENABLE_LIBOMPTARGET OFF"
            "OPENMP_STANDALONE_BUILD ON")
CPMAddPackage(
    NAME backtrace
    GITHUB_REPOSITORY "ianlancetaylor/libbacktrace"
    GIT_TAG d0f5e95a87a4d3e0a1ed6c069b5dae7cbab3ed2a
    DOWNLOAD_ONLY YES)
if(backtrace_ADDED)
    # https://go.googlesource.com/gollvm/+/refs/heads/master/cmake/modules/LibbacktraceUtils.cmake
    set(BACKTRACE_ELF_SIZE 64)
    set(HAVE_GETIPINFO 1)
    set(BACKTRACE_USES_MALLOC 0)
    set(BACKTRACE_SUPPORTS_THREADS 1)
    set(BACKTRACE_SUPPORTS_DATA 1)
    if(HAVE_SYNC_BOOL_COMPARE_AND_SWAP_4)
        if(HAVE_SYNC_BOOL_COMPARE_AND_SWAP_8)
            set(HAVE_SYNC_FUNCTIONS 1)
        endif()
    endif()
    # Generate backtrace-supported.h based on the above.
    configure_file(
        ${CMAKE_SOURCE_DIR}/scripts/backtrace-supported.h.cmake
        ${backtrace_SOURCE_DIR}/backtrace-supported.h)
    configure_file(
        ${CMAKE_SOURCE_DIR}/scripts/backtrace-config.h.cmake
        ${backtrace_SOURCE_DIR}/config.h)
    add_library(backtrace STATIC
        "${backtrace_SOURCE_DIR}/atomic.c"
        "${backtrace_SOURCE_DIR}/backtrace.c"
        "${backtrace_SOURCE_DIR}/dwarf.c"
        "${backtrace_SOURCE_DIR}/elf.c"
        "${backtrace_SOURCE_DIR}/fileline.c"
        "${backtrace_SOURCE_DIR}/mmapio.c"
        "${backtrace_SOURCE_DIR}/mmap.c"
        "${backtrace_SOURCE_DIR}/posix.c"
        "${backtrace_SOURCE_DIR}/print.c"
        "${backtrace_SOURCE_DIR}/simple.c"
        "${backtrace_SOURCE_DIR}/sort.c"
        "${backtrace_SOURCE_DIR}/state.c")
    target_include_directories(backtrace BEFORE PRIVATE "${backtrace_SOURCE_DIR}")
    set_target_properties(backtrace PROPERTIES
        COMPILE_FLAGS "-funwind-tables -D_GNU_SOURCE"
        POSITION_INDEPENDENT_CODE ON)
endif()
