#include <iostream>
#include <mpi.h>

#include "partial_cd.h"

int main(int argc, char *argv[])
{
  int rank = 42;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  partial_cd pcd(argv[1], rank);
  MPI_Finalize();
  return 0;
}
