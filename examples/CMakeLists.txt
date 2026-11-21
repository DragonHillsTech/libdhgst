# Set the output directory for the example binaries
set(EXAMPLES_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)

# Boost.GIL for gilview.hpp and the example classes
find_package(Boost REQUIRED)

# Classes shared by the examples, every example links all of them for simplicity
file(GLOB EXAMPLE_CLASSES_SOURCES "example-classes/*.cpp")
add_library(exampleclasses STATIC ${EXAMPLE_CLASSES_SOURCES})
target_compile_options(exampleclasses PRIVATE -Werror -Wall -Wextra)
target_include_directories(exampleclasses PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/example-classes)
target_link_libraries(exampleclasses PUBLIC dh::gst::libdhgst Boost::headers)

# Get all *.cpp files in the examples directory
file(GLOB EXAMPLE_SOURCES "*.cpp")

# Create an executable for each example source file
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
  # Extract the filename without extension
  get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)

  # Create the example executable
  add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})

  target_compile_options(${EXAMPLE_NAME} PRIVATE -Werror -Wall -Wextra)

  target_link_libraries(${EXAMPLE_NAME}
          dh::gst::libdhgst
          exampleclasses
  )

  # Set the output directory for the example binary
  set_target_properties(${EXAMPLE_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXAMPLES_OUTPUT_DIRECTORY}
  )
endforeach()
