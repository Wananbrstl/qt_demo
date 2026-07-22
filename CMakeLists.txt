cmake_minimum_required(VERSION 3.20)

project(cad_class_design_study VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "The dynamic-library case study targets Windows.")
endif()

option(CADSTUDY_BUILD_EXAMPLES "Build the runnable examples" ON)
option(CADSTUDY_BUILD_TESTS "Build the verification tests" ON)

add_library(cadstudy
  src/command_catalog.cpp
  src/dynamic_library.cpp
  src/parameter_graph.cpp
)

target_include_directories(cadstudy PUBLIC include)
target_compile_features(cadstudy PUBLIC cxx_std_17)

if(MSVC)
  target_compile_options(cadstudy PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
  target_compile_options(cadstudy PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

if(CADSTUDY_BUILD_EXAMPLES)
  add_executable(dynamic_library_example examples/dynamic_library/main.cpp)
  target_link_libraries(dynamic_library_example PRIVATE cadstudy)

  add_executable(declarative_commands_example examples/declarative_commands/main.cpp)
  target_link_libraries(declarative_commands_example PRIVATE cadstudy)

  add_executable(constraint_graph_example examples/constraint_graph/main.cpp)
  target_link_libraries(constraint_graph_example PRIVATE cadstudy)
endif()

if(CADSTUDY_BUILD_TESTS)
  enable_testing()
  add_executable(cadstudy_tests tests/cadstudy_tests.cpp)
  target_link_libraries(cadstudy_tests PRIVATE cadstudy)
  add_test(NAME cadstudy_tests COMMAND cadstudy_tests)
endif()
