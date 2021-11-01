cmake_minimum_required(VERSION 3.14)
project(CodonJupyter)

include(GNUInstallDirs)

# We generate the kernel.json file, given the installation prefix and the executable name
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/codon/kernel.json.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/codon/kernel.json")

set(CPM_DOWNLOAD_VERSION 0.32.3)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake...")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()
include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage(
    NAME libzmq
    VERSION 4.3.4
    URL https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz
    OPTIONS "WITH_PERF_TOOL OFF" 
            "ZMQ_BUILD_TESTS OFF" 
            "ENABLE_CPACK OFF"
            "BUILD_SHARED OFF"
            "WITH_LIBSODIUM OFF")
CPMAddPackage(
    NAME cppzmq
    URL https://github.com/zeromq/cppzmq/archive/refs/tags/v4.8.1.tar.gz
    VERSION 4.8.1
    OPTION "CPPZMQ_BUILD_TESTS OFF")
if(cppzmq_ADDED)
    set_target_properties(unit_tests PROPERTIES EXCLUDE_FROM_ALL ON)
endif()
CPMAddPackage(
    NAME xtl
    GITHUB_REPOSITORY "xtensor-stack/xtl"
    VERSION 0.7.3
    GIT_TAG 0.7.3
    OPTIONS "BUILD_TESTS OFF")
CPMAddPackage(
    NAME json
    GITHUB_REPOSITORY "nlohmann/json"
    VERSION 3.10.4)
CPMAddPackage(
    NAME xeus
    GITHUB_REPOSITORY "jupyter-xeus/xeus"
    VERSION 2.2.0
    GIT_TAG 2.2.0
    OPTIONS "BUILD_EXAMPLES OFF"
    "XEUS_BUILD_SHARED_LIBS OFF"
    "XEUS_STATIC_DEPENDENCIES ON")
if (xeus_ADDED)
    install(TARGETS nlohmann_json EXPORT xeus-targets)
endif()

set(CMAKE_CXX_STANDARD 14)
find_package(Threads)

set(CODON_SRC
    src/codon.cpp
    src/codon.h)
add_executable(codon-jupyter src/main.cpp ${CODON_SRC})
add_dependencies(codon-jupyter xeus-static)
target_link_libraries(codon-jupyter PRIVATE xeus-static Threads::Threads) # codonc)
set_target_properties(codon-jupyter PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

# Install kernel
install(TARGETS codon-jupyter RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set(XJUPYTER_DATA_DIR "share/jupyter" CACHE STRING "Jupyter data directory")
set(KERNELSPEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels)
install(DIRECTORY ${KERNELSPEC_DIR} DESTINATION ${XJUPYTER_DATA_DIR} PATTERN "*.in" EXCLUDE)
