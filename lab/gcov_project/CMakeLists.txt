# From: https://github.com/codecov/example-cpp11-cmake
cmake_minimum_required (VERSION 3.12..3.16)
project (Myproject VERSION 0.9 LANGUAGES CXX)

add_library(coverage_config INTERFACE)
option(CODE_COVERAGE "Enable coverage reporting" OFF)
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Add required flags (GCC & LLVM/Clang)
  target_compile_options(coverage_config INTERFACE
    -O0        # no optimization
    -g         # generate debug info
    --coverage # sets all required flags
    )
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    target_link_options(coverage_config INTERFACE --coverage)
  else()
    target_link_libraries(coverage_config INTERFACE --coverage)
  endif()
endif(CODE_COVERAGE)

add_executable(main src/main.cc)
target_link_libraries(main PUBLIC coverage_config)

enable_testing()
include(CTest)

add_test(NAME test0 COMMAND $<TARGET_FILE:main>)
