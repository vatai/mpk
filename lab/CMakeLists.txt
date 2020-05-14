# Flags not ported from old Makefile: -Wall -pedantic -g -O0
#
# -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_DEBUG_CSR_VALS

# CMakeLists.txt
cmake_minimum_required (VERSION 3.13)

### GOOGLETEST ###
enable_testing()
## See googletest github/homepage (installationg).
include(GoogleTest)

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
  ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
  EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories("${gtest_SOURCE_DIR}/include")
endif()

### Setup projectname ###
project (MPKLAB VERSION 0.1)

### Add compiler options ###
include(CheckCXXCompilerFlag)

function(enable_cxx_compiler_flag_if_supported flag)
  string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
  if(flag_already_set EQUAL -1)
    check_cxx_compiler_flag("${flag}" flag_supported)
    if(flag_supported)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
    endif()
    unset(flag_supported CACHE)
  endif()
endfunction()

enable_cxx_compiler_flag_if_supported("-Wall")
enable_cxx_compiler_flag_if_supported("-Wextra")
enable_cxx_compiler_flag_if_supported("-pedantic")

set(CMAKE_CXX_STANDARD 17)
# METIS
find_library(METIS metis)

# MPI
find_package(MPI REQUIRED)
# message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")

# add_definitions(-D_DEBUG_CSR_VALS)

add_executable(lapjv_test
  lapjv_test.cc
  ../mpk2/lapjv.c)
target_link_libraries(lapjv_test gtest_main)
add_test(NAME lapjv_test COMMAND lapjv_test)

add_executable(mpi_double_wait mpi_double_wait.cc)
target_link_libraries(mpi_double_wait MPI::MPI_CXX)

gtest_discover_tests(lapjv_test)
