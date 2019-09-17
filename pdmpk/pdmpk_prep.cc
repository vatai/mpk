//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-09-17

#include <iostream>
#include <sstream>
#include <mpi.h>

#include "partial_cd.h"

int main(int argc, char *argv[])
{
  int rank = 0;
  idx_t npart;
  partial_cd::level_t nlevels;

  // MPI_Init(&argc, &argv);
  // MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Get npart
  std::stringstream npart_ss(argv[2]);
  npart_ss >> npart;
  std::stringstream nlevels_ss(argv[3]);
  nlevels_ss >> nlevels;

  if (rank == 0) {
    partial_cd pcd(argv[1], rank, npart, nlevels);
  }

  // MPI_Finalize();
  return 0;
}
