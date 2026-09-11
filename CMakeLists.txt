cmake_minimum_required(VERSION 3.20)
project(miniwm VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(miniwm
  src/decoder.cpp
  src/module.cpp
  src/interpreter.cpp
)
target_include_directories(miniwm PUBLIC include)
target_compile_options(miniwm PRIVATE -Wall -Wextra -Wpedantic)

add_executable(miniwm_cli src/main.cpp)
target_link_libraries(miniwm_cli PRIVATE miniwm)
set_target_properties(miniwm_cli PROPERTIES OUTPUT_NAME miniwm)

enable_testing()
add_executable(miniwm_smoke tests/smoke.cpp)
target_link_libraries(miniwm_smoke PRIVATE miniwm)
add_test(NAME miniwm_smoke COMMAND miniwm_smoke)

option(MINIWM_BUILD_TESTS "Build GoogleTest unit tests" ON)
if(MINIWM_BUILD_TESTS)
  find_package(GTest QUIET)
  if(GTest_FOUND)
    add_executable(miniwm_tests
      tests/decoder_test.cpp
      tests/interpreter_test.cpp
    )
    target_link_libraries(miniwm_tests PRIVATE miniwm GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(miniwm_tests)
  else()
    message(STATUS "GoogleTest not found; building the dependency-free smoke test only")
  endif()
endif()
