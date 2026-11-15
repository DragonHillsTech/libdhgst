# Builds the Doxygen API documentation.
#
# add_doxygen_docs(INPUT_DIR <dir> HEADERS <header>...)
#   Generates the HTML documentation of the headers in <dir> (recursively, *.hpp) with the
#   Doxyfile.in next to this file. The target docs is part of "all", the output goes to
#   <build>/docs/doxygen/html and is installed to DATAROOTDIR/doc/<project> (component documentation).
#   HEADERS: the documentation is regenerated when one of them changes.
#   Configuring fails if Doxygen is not found.

# CMAKE_CURRENT_LIST_DIR inside the function would be the directory of the caller
set(DOCUMENTATION_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

include(GNUInstallDirs)

function(add_doxygen_docs)
  cmake_parse_arguments(ARG "" "INPUT_DIR" "HEADERS" ${ARGN})
  if(NOT ARG_INPUT_DIR)
    message(FATAL_ERROR "add_doxygen_docs: INPUT_DIR is required")
  endif()

  find_package(Doxygen)
  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is required to build documentation")
  endif()

  # used by Doxyfile.in
  set(DOXYGEN_INPUT_DIR "${ARG_INPUT_DIR}")
  set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen")
  set(DOXYGEN_INDEX_FILE "${DOXYGEN_OUTPUT_DIR}/html/index.html")

  configure_file(
    "${DOCUMENTATION_CMAKE_DIR}/Doxyfile.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    @ONLY
  )
  add_custom_command(
    OUTPUT "${DOXYGEN_INDEX_FILE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${DOXYGEN_OUTPUT_DIR}"
    COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    MAIN_DEPENDENCY "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    DEPENDS
      ${ARG_HEADERS}
    COMMENT "Generating Doxygen documentation"
    VERBATIM
  )
  add_custom_target(
    docs
    ALL
    DEPENDS "${DOXYGEN_INDEX_FILE}"
  )
  install(
    DIRECTORY "${DOXYGEN_OUTPUT_DIR}/html"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${PROJECT_NAME}"
    COMPONENT documentation
  )
endfunction()
