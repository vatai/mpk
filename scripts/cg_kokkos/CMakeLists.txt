cmake_minimum_required (VERSION 3.12..3.16)
project (CG_KOKKOS VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Kokkos)
add_executable(cg_kokkos cg_kokkos.cc)
target_link_libraries(cg_kokkos Kokkos::kokkos)
