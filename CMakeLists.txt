cmake_minimum_required(VERSION 3.10)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/ReadVersion.cmake)
read_version(${CMAKE_CURRENT_LIST_DIR}/VERSION.txt LIBDHGST_VERSION)

project(libdhgst VERSION ${LIBDHGST_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)


# Options
option(BUILD_TESTS "Build the unit tests" OFF)
option(BUILD_EXAMPLES "Build example programs" OFF)
option(BUILD_DOCUMENTATION "Build Doxygen documentation" OFF)
# Off by default: .clang-tidy turns all warnings into errors, and other clang-tidy versions report
# different findings
option(ENABLE_CLANG_TIDY "Check the library sources with clang-tidy while compiling" OFF)

message(STATUS "")
message(STATUS "Project options:")
message(STATUS "  BUILD_TESTS         = ${BUILD_TESTS}")
message(STATUS "  BUILD_EXAMPLES      = ${BUILD_EXAMPLES}")
message(STATUS "  BUILD_DOCUMENTATION = ${BUILD_DOCUMENTATION}")
message(STATUS "  ENABLE_CLANG_TIDY   = ${ENABLE_CLANG_TIDY}")
message(STATUS "")

# For running clang-tidy by hand and for IDEs, can still be turned off with -D...=OFF. CMake
# creates the cache entry empty, so empty means the user didn't set it. Only at top level: a parent
# project that includes this one with add_subdirectory decides for itself.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR
   AND "${CMAKE_EXPORT_COMPILE_COMMANDS}" STREQUAL "")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# Set the source and header files
set(SOURCES
  src/appsink.cpp
  src/appsrc.cpp
  src/bin.cpp
  src/bus.cpp
  src/element.cpp
  src/elementfactory.cpp
  src/helpers.cpp
  src/messageparser.cpp
  src/object.cpp
  src/pluginfeature.cpp
  src/pipeline.cpp
  src/signalconnection.cpp
)

set(HEADERS
  src/appsink.hpp
  src/appsrc.hpp
  src/bin.hpp
  src/bus.hpp
  src/element.hpp
  src/elementfactory.hpp
  src/gilview.hpp
  src/gvaluetraits.hpp
  src/helpers.hpp
  src/object.hpp
  src/objecttraits.hpp
  src/messageparser.hpp
  src/pipeline.hpp
  src/pluginfeature.hpp
  src/sharedptrs.hpp
  src/signalconnection.hpp
  src/transfertype.hpp
  src/typetraits.hpp
)

if(BUILD_DOCUMENTATION)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/Documentation.cmake)
  # the public headers in src/
  add_doxygen_docs(
    INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src"
    HEADERS ${HEADERS}
  )
endif()

# Find required packages
# The imported targets are part of the exported link interface, so libdhgstConfig.cmake.in must
# create them with the same names. The DHGST_ prefix avoids clashes with the consumer's own variables.
find_package(PkgConfig REQUIRED)
pkg_check_modules(DHGST_GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.14)
pkg_check_modules(DHGST_GST_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
pkg_check_modules(DHGST_GST_VIDEO REQUIRED IMPORTED_TARGET gstreamer-video-1.0)

# Boost is not a dependency of the library: only the header-only gilview.hpp uses it (Boost.GIL),
# so users of that header link Boost themselves. The tests find Boost in tests/CMakeLists.txt.

# Create the shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES})
# Same name as the exported target, so in-tree users can link it the same way
add_library(dh::gst::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Only the library: the tests and examples include third-party headers that .clang-tidy would check
if(ENABLE_CLANG_TIDY)
  # No REQUIRED, which needs CMake 3.18. The versioned name first, the unversioned clang-tidy may be
  # an older one installed next to it.
  find_program(DHGST_CLANG_TIDY_EXE NAMES clang-tidy-21 clang-tidy)
  if(NOT DHGST_CLANG_TIDY_EXE)
    message(FATAL_ERROR "ENABLE_CLANG_TIDY is ON, but clang-tidy (21 or newer) was not found."
      " Install it or set -DDHGST_CLANG_TIDY_EXE=/path/to/clang-tidy")
  endif()
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${DHGST_CLANG_TIDY_EXE}")
endif()

# PUBLIC: the headers use C++17 features, so consumers need at least C++17, too
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/dh/gst>
)

# Add version information to the library
# Automatically set SOVERSION based on the project major version
set(SOVERSION_MAJOR ${PROJECT_VERSION_MAJOR})

set_target_properties(${PROJECT_NAME} PROPERTIES
  OUTPUT_NAME dhgst
  VERSION ${PROJECT_VERSION}
  SOVERSION ${SOVERSION_MAJOR}
)

# Link libraries
# PUBLIC: the public headers include the GStreamer headers
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    PkgConfig::DHGST_GSTREAMER
    PkgConfig::DHGST_GST_APP
    PkgConfig::DHGST_GST_VIDEO
)

# Installation directives

install(TARGETS ${PROJECT_NAME}
  EXPORT libdhgstTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dh/gst
)

# Create and install the pkg-config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libdhgst.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/libdhgst.pc @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libdhgst.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Export the library for other CMake projects
install(EXPORT libdhgstTargets
  FILE libdhgstTargets.cmake
  NAMESPACE dh::gst::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libdhgst
)

# Add the include directory to the search path
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/libdhgstConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/libdhgstConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/libdhgstConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libdhgst
  PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/libdhgstConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/libdhgstConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libdhgst
)

# Add the tests directory if BUILD_TESTS is ON
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

