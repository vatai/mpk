
find_library(METIS metis)
find_package(MPI REQUIRED)
# message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")

# Get the current working branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )
add_definitions("-DGIT_BRANCH=\"${GIT_BRANCH}\"")

# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

# Check if the branch is dirty
execute_process(
  COMMAND "git status > /dev/null 2&>1"
  RESULT_VARIABLE GIT_DIRTY_WD
  )

if(GIT_DIRTY_WD)
add_definitions("-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")
else()
add_definitions("-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}-dirty\"")
endif()

add_library(args OBJECT args.h args.cc)

add_library(csr OBJECT csr.h csr.cc)
add_library(mcsr OBJECT mcsr.h mcsr.cc)
add_library(mpi_buffers OBJECT mpi_buffers.h mpi_buffers.cc)
add_library(pdmpk_buffers OBJECT pdmpk_buffers.h pdmpk_buffers.cc)
add_library(lapjv OBJECT lapjv.h lapjv.c)
add_library(results OBJECT results.h results.cc)

add_library(buffers buffers.h buffers.cc
  $<TARGET_OBJECTS:mpi_buffers>
  $<TARGET_OBJECTS:results>
  $<TARGET_OBJECTS:mcsr>)
target_link_libraries(buffers MPI::MPI_CXX)

add_library(comm_comp_patterns comm_comp_patterns.h comm_comp_patterns.cc
  $<TARGET_OBJECTS:csr>
  $<TARGET_OBJECTS:pdmpk_buffers>
  $<TARGET_OBJECTS:lapjv>)
target_link_libraries(comm_comp_patterns buffers ${METIS})

add_executable(pdmpk_prep pdmpk_prep.cc $<TARGET_OBJECTS:args>)
target_link_libraries(pdmpk_prep comm_comp_patterns)

add_executable(pdmpk_exec pdmpk_exec.cc
  $<TARGET_OBJECTS:args>)
target_link_libraries(pdmpk_exec buffers MPI::MPI_CXX ${METIS})

add_executable(pdmpk_test pdmpk_test.cc
  $<TARGET_OBJECTS:args>
  $<TARGET_OBJECTS:csr>
  $<TARGET_OBJECTS:results>)

target_link_libraries(pdmpk_test ${METIS})
