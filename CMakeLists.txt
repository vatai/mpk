# Flags not ported from old Makefile: -Wall -pedantic -g -O0
#
# -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_DEBUG_CSR_VALS

# CMakeLists.txt
cmake_minimum_required (VERSION 3.15)

## GOOGLETEST BEGIN
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
## GOOGLETEST END

# Setup projectname
project (PDMPK VERSION 0.9)

set (CMAKE_CXX_STANDARD 17)

# METIS
find_library(METIS metis)

# MPI
# set(MPI_GUESS_LIBRARY_NAME "ortecc")
set(MPI_C_COMPILER "/usr/bin/mpicc")
set(MPI_CXX_COMPILER "/usr/bin/mpic++")
find_package(MPI REQUIRED)
# message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")

add_executable (pdmpk_prep
  "pdmpk/pdmpk_prep.cc"
  "pdmpk/partial_cd.cc"
  "pdmpk/csr_t.cc"
  "pdmpk/buffers_t.cc"
  "pdmpk/mpi_bufs_t.cc"
  "pdmpk/mcsr_t.cc"
  "pdmpk/pdmpk_bufs_t.cc"
  )

target_link_libraries(pdmpk_prep "${METIS}" MPI::MPI_CXX)

add_executable (pdmpk_exec
  "pdmpk/pdmpk_exec.cc"
  "pdmpk/buffers_t.cc"
  "pdmpk/mpi_bufs_t.cc"
  "pdmpk/mcsr_t.cc"
  )

target_link_libraries(pdmpk_exec "${METIS}" MPI::MPI_CXX)
