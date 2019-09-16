#include <iostream>
#include <sstream>
#include <mpi.h>

#include "partial_cd.h"

int main(int argc, char *argv[])
{
  int rank, npart;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Get npart
  std::stringstream ss(argv[2]);
  ss >> npart;

  partial_cd pcd(argv[1], rank, npart);

  MPI_Finalize();
  return 0;
}
