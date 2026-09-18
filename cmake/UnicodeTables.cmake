find_package(Python3 3.8 COMPONENTS Interpreter REQUIRED)

set(CODON_UNICODE_VERSION "16.0.0")
set(CODON_UNICODE_ARCHIVE
  "${CMAKE_SOURCE_DIR}/scripts/unicode/tables-${CODON_UNICODE_VERSION}.tar.gz"
  CACHE FILEPATH "Local, checksum-verified Unicode table archive")
set(CODON_UNICODE_MANIFEST "${CMAKE_SOURCE_DIR}/scripts/unicode/manifest.json")
set(CODON_UNICODE_GENERATORS
  "${CMAKE_SOURCE_DIR}/scripts/unicode_tables.py"
  "${CMAKE_SOURCE_DIR}/scripts/unicode/inputs.json"
  "${CMAKE_SOURCE_DIR}/scripts/generate_unicode_data.py"
  "${CMAKE_SOURCE_DIR}/scripts/generate_unicode_normalization_data.py"
  "${CMAKE_SOURCE_DIR}/scripts/generate_single_byte_codecs.py"
  "${CMAKE_SOURCE_DIR}/scripts/generate_float_dtoa_tables.py"
)
set(CODON_UNICODE_GENERATED_FILES
  "${CMAKE_BINARY_DIR}/stdlib/internal/unicode/generated/properties.codon"
  "${CMAKE_BINARY_DIR}/stdlib/internal/unicode/generated/single_byte_codecs.codon"
  "${CMAKE_BINARY_DIR}/stdlib/internal/unicode/generated/normalization.codon"
  "${CMAKE_BINARY_DIR}/stdlib/internal/unicode/generated/metadata.codon"
  "${CMAKE_BINARY_DIR}/stdlib/internal/unicode/generated/names.codon"
  "${CMAKE_BINARY_DIR}/stdlib/internal/numeric/generated/float_dtoa_tables.codon"
)
add_custom_command(
  OUTPUT ${CODON_UNICODE_GENERATED_FILES}
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/unicode_tables.py"
          extract --archive "${CODON_UNICODE_ARCHIVE}"
          --output-dir "${CMAKE_BINARY_DIR}/stdlib"
  DEPENDS ${CODON_UNICODE_GENERATORS} "${CODON_UNICODE_MANIFEST}"
          "${CODON_UNICODE_ARCHIVE}"
  COMMENT "Verifying and unpacking Unicode ${CODON_UNICODE_VERSION} tables"
  VERBATIM
)
add_custom_target(unicode_data DEPENDS ${CODON_UNICODE_GENERATED_FILES})

add_custom_target(unicode_data_verify
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/unicode_tables.py"
    verify --archive "${CODON_UNICODE_ARCHIVE}"
  VERBATIM
)
set(CODON_UNICODE_UCD_DIR "${CMAKE_BINARY_DIR}/unicode-${CODON_UNICODE_VERSION}"
    CACHE PATH "Checksum-pinned UCD inputs for explicit table regeneration")
add_custom_target(unicode_data_regenerate
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/unicode_tables.py"
    regenerate --ucd-dir "${CODON_UNICODE_UCD_DIR}"
    --output-dir "${CMAKE_BINARY_DIR}/stdlib"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/unicode_tables.py"
    pack --input-dir "${CMAKE_BINARY_DIR}/stdlib"
  COMMENT "Regenerating Unicode tables and updating the source bundle"
  VERBATIM
)
