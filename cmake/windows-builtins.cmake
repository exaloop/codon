# Compiler helpers come from the same LLVM distribution as the compiler.
find_library(COMPILER_RT_LIB
  NAMES clang_rt.builtins-x86_64
  PATHS "${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION_MAJOR}/lib/windows"
        "${LLVM_LIBRARY_DIR}/clang/${LLVM_PACKAGE_VERSION}/lib/windows"
  NO_DEFAULT_PATH REQUIRED)
find_program(CODON_LLVM_NM NAMES llvm-nm
  HINTS "${LLVM_TOOLS_BINARY_DIR}" REQUIRED)

# CMake's automatic DLL exports scan codonrt's objects, not static archives.
# Derive the additional exports from compiler-rt itself, so new helpers do not
# require a second symbol list here. Exporting them also pulls in their objects.
execute_process(COMMAND "${CODON_LLVM_NM}" --defined-only --extern-only
                        --format=posix "${COMPILER_RT_LIB}"
  OUTPUT_VARIABLE _codon_builtin_symbols
  COMMAND_ERROR_IS_FATAL ANY)
string(REPLACE "\n" ";" _codon_builtin_lines "${_codon_builtin_symbols}")
set(_codon_builtin_exports "EXPORTS\n")
foreach(_line IN LISTS _codon_builtin_lines)
  if(_line MATCHES "^([A-Za-z_][A-Za-z_0-9]*) T ")
    set(_symbol "${CMAKE_MATCH_1}")
    # Keep Codon's GC-aware TLS and Windows' native SEH personality.
    if(NOT _symbol MATCHES "^(__emutls_|__gcc_personality_v0$)")
      string(APPEND _codon_builtin_exports "${_symbol}\n")
      list(APPEND _codon_builtin_names "${_symbol}")
    endif()
  endif()
endforeach()
set(_codon_required_builtins __divti3 __udivti3 __truncdfhf2 __truncsfbf2)
list(APPEND _codon_required_builtins __addtf3 __multf3 __eqtf2
       __netf2 __extenddftf2 __trunctfdf2)
foreach(_symbol IN LISTS _codon_required_builtins)
  if(NOT _symbol IN_LIST _codon_builtin_names)
    message(FATAL_ERROR "${COMPILER_RT_LIB} is missing ${_symbol}. Build the Windows builtins from Codon's LLVM fork.")
  endif()
endforeach()
set(_codon_builtin_def "${CMAKE_CURRENT_BINARY_DIR}/compiler-rt-exports.def")
file(CONFIGURE OUTPUT "${_codon_builtin_def}" CONTENT "${_codon_builtin_exports}" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${COMPILER_RT_LIB}")
target_sources(codonrt PRIVATE "${_codon_builtin_def}")
target_link_libraries(codonrt PRIVATE "${COMPILER_RT_LIB}")
message(STATUS "Using compiler-rt builtins: ${COMPILER_RT_LIB}")
