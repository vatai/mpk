cmake_minimum_required(VERSION 2.7)
project(MPI_labtest1)

find_package(MPI REQUIRED)

include_directories(${MPI_INCLUDE_PATH})

add_executable(lt1 mpi_labtest1.cpp)
target_link_libraries(lt1 ${MPI_LIBRARIES})

if(MPI_COMPILE_FLAGS)
  set_target_properties(lt1 PROPERTIES
    COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
  set_target_properties(lt1 PROPERTIES
    LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()
