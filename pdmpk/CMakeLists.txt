# Flags not ported from old Makefile: -Wall -pedantic -g -O0
#
# -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_DEBUG_CSR_VALS

cmake_minimum_required (VERSION 3.12..3.16)
project (PDMPK VERSION 0.9 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)

enable_testing()

### Doxygen ###
# From: https://vicrucann.github.io/tutorials/quick-cmake-doxygen/
# first we can indicate the documentation build as an option and set it to ON by default
option(BUILD_DOC "Build documentation" ON)

# check if Doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND AND BUILD_DOC)
  # Warn only in release.
  if (CMAKE_BUILD_TYPE MATCHES "^[Rr]elease")
    set(CMAKE_DOXYGEN_WARN_IF_UNDOCUMENTED "YES")
  else()
    set(CMAKE_DOXYGEN_WARN_IF_UNDOCUMENTED "NO")
  endif()
  # set input and output files
  set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/../docs/Doxyfile.in)
  set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  # request to configure the file
  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
  message("Doxygen build started")

  # note the option ALL which allows to build the docs together with the application
  add_custom_target(doc_doxygen ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
elseif (BUILD_DOC)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND AND BUILD_DOC)

### GOOGLETEST ###
## See googletest github/homepage (installationg).
# # Download and unpack googletest at configure time
# configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
# execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
#   RESULT_VARIABLE result
#   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
# if(result)
#   message(FATAL_ERROR "CMake step for googletest failed: ${result}")
# endif()
# execute_process(COMMAND ${CMAKE_COMMAND} --build .
#   RESULT_VARIABLE result
#   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
# if(result)
#   message(FATAL_ERROR "Build step for googletest failed: ${result}")
# endif()

# # Prevent overriding the parent project's compiler/linker
# # settings on Windows
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# # Add googletest directly to our build. This defines
# # the gtest and gtest_main targets.
# add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
#                  ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
#                  EXCLUDE_FROM_ALL)

# # The gtest/gtest_main targets carry header search path
# # dependencies automatically when using CMake 2.8.11 or
# # later. Otherwise we have to add them here ourselves.
# if (CMAKE_VERSION VERSION_LESS 2.8.11)
#   include_directories("${gtest_SOURCE_DIR}/include")
# endif()

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

find_library(METIS metis)
find_package(MPI REQUIRED)
# message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")

add_library(args args.h args.cc)
add_library(csr csr.h csr.cc)
add_library(mcsr mcsr.h mcsr.cc)
add_library(mpi_buffers mpi_buffers.h mpi_buffers.cc)
add_library(pdmpk_buffers pdmpk_buffers.h pdmpk_buffers.cc)
add_library(lapjv lapjv.h lapjv.c)
add_library(results results.h results.cc)

add_library(buffers buffers.h buffers.cc)
target_link_libraries(buffers mpi_buffers results mcsr MPI::MPI_CXX)

add_library(comm_comp_patterns comm_comp_patterns.h comm_comp_patterns.cc)
target_link_libraries(comm_comp_patterns buffers csr pdmpk_buffers lapjv ${METIS})

add_executable(pdmpk_prep pdmpk_prep.cc)
target_link_libraries(pdmpk_prep args comm_comp_patterns buffers)

add_executable(pdmpk_exec pdmpk_exec.cc)
target_link_libraries(pdmpk_exec MPI::MPI_CXX buffers)

add_executable(pdmpk_test pdmpk_test.cc)
target_link_libraries(pdmpk_test args csr results)

set(TESTS_DIR "${CMAKE_SOURCE_DIR}/../tests")

add_test(NAME m5p4-4-4 COMMAND "${TESTS_DIR}/m5p4-4-4.sh")
