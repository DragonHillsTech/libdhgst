cmake_minimum_required(VERSION 3.10)
project(libdhgst LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

# Option to build tests
option(BUILD_TESTS "Build the unit tests" ON)

# Find GStreamer package
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0>=1.14)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
include_directories(${GSTREAMER_INCLUDE_DIRS})
link_directories(${GSTREAMER_LIBRARY_DIRS})

# Find Boost package with Signals2 component
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# Find spdlog package
find_package(spdlog REQUIRED)

# Include source directory for header files
include_directories(src)

# Add library source files
add_library(libdhgst SHARED
        src/element.cpp
        src/element.hpp
        src/transfertype.hpp
        src/sharedptrs.hpp
)

# Link against GStreamer and spdlog
target_link_libraries(libdhgst ${GSTREAMER_LIBRARIES} ${GST_APP_LIBRARIES} spdlog::spdlog)

# Add the tests directory if BUILD_TESTS is ON
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
