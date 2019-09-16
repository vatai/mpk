#include <iostream>
#include <sstream>
#include <mpi.h>

#include "partial_cd.h"

int main(int argc, char *argv[])
{
  int rank, npart, nlevels;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Get npart
  std::stringstream ss(argv[2]);
  ss >> npart;
  ss.clear();
  ss << argv[3];
  ss >> nlevels;

  partial_cd pcd(argv[1], rank, npart, nlevels);

  MPI_Finalize();
  return 0;
}
