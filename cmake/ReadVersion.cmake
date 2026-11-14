# Reads the project version from a file.
#
# read_version(<file> <out_var>)
#   Reads <file>, strips surrounding whitespace and stores the version in <out_var>.
#   The version must have the form MAJOR.MINOR.PATCH, otherwise configuring fails.
#   CMake reconfigures automatically when <file> changes.
#
# Must be usable before project(), so it only uses commands available without a language.

function(read_version file out_var)
  if(NOT EXISTS "${file}")
    message(FATAL_ERROR "Version file not found: ${file}")
  endif()

  file(READ "${file}" version)
  string(STRIP "${version}" version)

  if(NOT version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "Invalid version '${version}' in ${file}, expected MAJOR.MINOR.PATCH")
  endif()

  # re-run CMake when the version file changes
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${file}")

  set(${out_var} "${version}" PARENT_SCOPE)
endfunction()
