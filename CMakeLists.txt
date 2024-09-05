cmake_minimum_required(VERSION 3.10)
project(libdhgst VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

# Option to build tests
option(BUILD_TESTS "Build the unit tests" ON)

# Set the source and header files
set(SOURCES
  src/element.cpp
)
set(HEADERS
  src/element.hpp
  src/transfertype.hpp
  src/sharedptrs.hpp
)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0>=1.14)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
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
        OUTPUT_NAME dh
        VERSION ${PROJECT_VERSION}
        SOVERSION ${SOVERSION_MAJOR}
)

# Link libraries
target_link_libraries(libdhgst ${GSTREAMER_LIBRARIES} ${GST_APP_LIBRARIES} spdlog::spdlog)

# Installation directives
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
        EXPORT libdhgstTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dh/libdhgst
)

# Add the tests directory if BUILD_TESTS is ON
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
