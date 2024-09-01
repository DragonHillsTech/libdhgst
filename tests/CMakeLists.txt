

include_directories(../src)

# Include Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Set the output directory for the test binaries
set(TEST_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

# Get all gtest_*.cpp files in the tests directory
file(GLOB TEST_SOURCES "gtest_*.cpp")

# Create a test executable for each test source file
foreach(TEST_SOURCE ${TEST_SOURCES})
  # Extract the filename without extension
  get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

  # Create the test executable
  add_executable(${TEST_NAME} ${TEST_SOURCE})

  # Link the test executable with libdhgst and GTest
  target_link_libraries(${TEST_NAME} libdhgst GTest::GTest GTest::Main ${GSTREAMER_LIBRARIES})

  # Set the output directory for the test binary
  set_target_properties(${TEST_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIRECTORY}
  )

  # Add the test to CTest
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

