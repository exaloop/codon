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
    NAME xz
    GIT_REPOSITORY "https://git.tukaani.org/xz.git"
    VERSION 5.2.5
    OPTIONS "BUILD_SHARED_LIBS OFF"
            "CMAKE_POSITION_INDEPENDENT_CODE ON")
set_target_properties(xz PROPERTIES EXCLUDE_FROM_ALL ON)
set_target_properties(xzdec PROPERTIES EXCLUDE_FROM_ALL ON)
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
    NAME htslib
    VERSION 1.13
    URL "https://github.com/samtools/htslib/releases/download/1.13/htslib-1.13.tar.bz2"
    DOWNLOAD_ONLY YES)
if(htslib_ADDED)
    configure_file(
        ${CMAKE_SOURCE_DIR}/scripts/htslib-config.h.cmake
        ${htslib_SOURCE_DIR}/config.h
        COPYONLY)
    write_file(${htslib_SOURCE_DIR}/version.h
        "#define HTS_VERSION_TEXT \"${CPM_PACKAGE_htslib_VERSION}\"")
    write_file(${htslib_SOURCE_DIR}/config_vars.h
        "#define HTS_CC \"\"\n"
        "#define HTS_CPPFLAGS \"\"\n"
        "#define HTS_CFLAGS \"\"\n"
        "#define HTS_LDFLAGS \"\"\n"
        "#define HTS_LIBS \"\"\n")
    add_library(htslib STATIC
        "${htslib_SOURCE_DIR}/kfunc.c"
        "${htslib_SOURCE_DIR}/kstring.c"
        "${htslib_SOURCE_DIR}/bcf_sr_sort.c"
        "${htslib_SOURCE_DIR}/bgzf.c"
        "${htslib_SOURCE_DIR}/errmod.c"
        "${htslib_SOURCE_DIR}/faidx.c"
        "${htslib_SOURCE_DIR}/header.c"
        "${htslib_SOURCE_DIR}/hfile.c"
        "${htslib_SOURCE_DIR}/hts.c"
        "${htslib_SOURCE_DIR}/hts_expr.c"
        "${htslib_SOURCE_DIR}/hts_os.c"
        "${htslib_SOURCE_DIR}/md5.c"
        "${htslib_SOURCE_DIR}/multipart.c"
        "${htslib_SOURCE_DIR}/probaln.c"
        "${htslib_SOURCE_DIR}/realn.c"
        "${htslib_SOURCE_DIR}/regidx.c"
        "${htslib_SOURCE_DIR}/region.c"
        "${htslib_SOURCE_DIR}/sam.c"
        "${htslib_SOURCE_DIR}/synced_bcf_reader.c"
        "${htslib_SOURCE_DIR}/vcf_sweep.c"
        "${htslib_SOURCE_DIR}/tbx.c"
        "${htslib_SOURCE_DIR}/textutils.c"
        "${htslib_SOURCE_DIR}/thread_pool.c"
        "${htslib_SOURCE_DIR}/vcf.c"
        "${htslib_SOURCE_DIR}/vcfutils.c"
        "${htslib_SOURCE_DIR}/cram/cram_codecs.c"
        "${htslib_SOURCE_DIR}/cram/cram_decode.c"
        "${htslib_SOURCE_DIR}/cram/cram_encode.c"
        "${htslib_SOURCE_DIR}/cram/cram_external.c"
        "${htslib_SOURCE_DIR}/cram/cram_index.c"
        "${htslib_SOURCE_DIR}/cram/cram_io.c"
        "${htslib_SOURCE_DIR}/cram/cram_stats.c"
        "${htslib_SOURCE_DIR}/cram/mFILE.c"
        "${htslib_SOURCE_DIR}/cram/open_trace_file.c"
        "${htslib_SOURCE_DIR}/cram/pooled_alloc.c"
        "${htslib_SOURCE_DIR}/cram/string_alloc.c"
        "${htslib_SOURCE_DIR}/htscodecs/htscodecs/arith_dynamic.c"
        "${htslib_SOURCE_DIR}/htscodecs/htscodecs/fqzcomp_qual.c"
        "${htslib_SOURCE_DIR}/htscodecs/htscodecs/htscodecs.c"
        "${htslib_SOURCE_DIR}/htscodecs/htscodecs/pack.c"
        "${htslib_SOURCE_DIR}/htscodecs/htscodecs/rANS_static4x16pr.c"
        "${htslib_SOURCE_DIR}/htscodecs/htscodecs/rANS_static.c"
        "${htslib_SOURCE_DIR}/htscodecs/htscodecs/rle.c"
        "${htslib_SOURCE_DIR}/htscodecs/htscodecs/tokenise_name3.c")
    target_include_directories(htslib BEFORE PRIVATE "${htslib_SOURCE_DIR}" "${bz2_SOURCE_DIR}" "${xz_SOURCE_DIR}/src/liblzma/api")
    set_target_properties(htslib PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        VISIBILITY_INLINES_HIDDEN ON)
endif()
