
find_library(METIS metis)
find_package(MPI REQUIRED)
# message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")

add_library(args OBJECT args.h args.cc)
add_library(csr OBJECT csr.h csr.cc)
add_library(mcsr OBJECT mcsr.h mcsr.cc)
add_library(mpi_buffers OBJECT mpi_buffers.h mpi_buffers.cc)
add_library(pdmpk_buffers OBJECT pdmpk_buffers.h pdmpk_buffers.cc)
add_library(lapjv OBJECT lapjv.h lapjv.c)
add_library(results OBJECT results.h results.cc)
add_library(buffers OBJECT buffers.h buffers.cc)
add_library(comm_comp_patterns OBJECT comm_comp_patterns.h comm_comp_patterns.cc)

add_executable(pdmpk_prep pdmpk_prep.cc $<TARGET_OBJECTS:args>
  $<TARGET_OBJECTS:comm_comp_patterns>
  $<TARGET_OBJECTS:lapjv>
  $<TARGET_OBJECTS:mpi_buffers>
  $<TARGET_OBJECTS:pdmpk_buffers>
  $<TARGET_OBJECTS:buffers>
  $<TARGET_OBJECTS:csr>
  $<TARGET_OBJECTS:mcsr>
  $<TARGET_OBJECTS:results>)
target_link_libraries(pdmpk_prep comm_comp_patterns MPI::MPI_CXX ${METIS})

add_executable(pdmpk_exec pdmpk_exec.cc
  $<TARGET_OBJECTS:args>
  $<TARGET_OBJECTS:buffers>
  $<TARGET_OBJECTS:mpi_buffers>
  $<TARGET_OBJECTS:mcsr>
  $<TARGET_OBJECTS:results>)
target_link_libraries(pdmpk_exec MPI::MPI_CXX ${METIS})

add_executable(pdmpk_test pdmpk_test.cc
  $<TARGET_OBJECTS:args>
  $<TARGET_OBJECTS:csr>
  $<TARGET_OBJECTS:results>)

target_link_libraries(pdmpk_test ${METIS})
