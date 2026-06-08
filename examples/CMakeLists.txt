include_directories(../src)

# Set the output directory for the test binaries
set(EXAMPLES_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)

file(GLOB EXAMPLE_SOURCES "*.cpp")


# Create a test executable for each test source file
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
  # Extract the filename without extension
  get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)

  # Create the test executable
  add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})

  target_link_libraries(${EXAMPLE_NAME}
          libdhgst ${GSTREAMER_LIBRARIES}
  )

  # Set the output directory for the test binary
  set_target_properties(${EXAMPLE_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXAMPLES_OUTPUT_DIRECTORY}
  )
endforeach()

