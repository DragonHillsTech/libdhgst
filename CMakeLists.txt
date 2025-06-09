cmake_minimum_required(VERSION 3.10)
project(libdhgst VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

# Options
option(BUILD_TESTS "Build the unit tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)

# Set the source and header files
set(SOURCES
  src/bin.cpp
  src/bus.cpp
  src/element.cpp
  src/elementfactory.cpp
  src/helpers.cpp
  src/messageparser.cpp
  src/object.cpp
  src/pluginfeature.cpp
  src/pipeline.cpp
)

set(HEADERS
  src/bin.hpp
  src/bus.hpp
  src/element.hpp
  src/elementfactory.hpp
  src/helpers.hpp
  src/object.hpp
  src/objecttraits.hpp
  src/messageparser.hpp
  src/pipeline.hpp
  src/pluginfeature.cpp
  src/sharedptrs.hpp
  src/transfertype.hpp
  src/typetraits.hpp
)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0>=1.14)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)
include_directories(${GSTREAMER_INCLUDE_DIRS})
link_directories(${GSTREAMER_LIBRARY_DIRS})

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(spdlog REQUIRED)

# Include source directory for header files
include_directories(src)

# Create the shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Add version information to the library
# Automatically set SOVERSION based on the project major version
set(SOVERSION_MAJOR ${PROJECT_VERSION_MAJOR})

set_target_properties(${PROJECT_NAME} PROPERTIES
  OUTPUT_NAME dhgst
  VERSION ${PROJECT_VERSION}
  SOVERSION ${SOVERSION_MAJOR}
)

# Link libraries
target_link_libraries(libdhgst
  ${GSTREAMER_LIBRARIES}
  ${GST_APP_LIBRARIES}
  ${GSTREAMER_VIDEO_LIBRARIES}
  spdlog::spdlog
)

# Installation directives
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
  EXPORT libdhgstTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dh/libdhgst
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
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/libdhgstConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/libdhgstConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libdhgst
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

