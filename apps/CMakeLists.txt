
find_library(METIS metis)
find_package(MPI REQUIRED)
# message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")

add_executable(prep prep.cc $<TARGET_OBJECTS:args>)
target_link_libraries(prep PUBLIC comm_comp_patterns coverage_config)

add_executable(exec exec.cc $<TARGET_OBJECTS:args>)
target_link_libraries(exec PUBLIC buffers MPI::MPI_CXX ${METIS} coverage_config)

add_executable(check check.cc
  $<TARGET_OBJECTS:args>
  $<TARGET_OBJECTS:csr>
  $<TARGET_OBJECTS:results>)
target_link_libraries(check PUBLIC ${METIS} MPI::MPI_CXX coverage_config)
