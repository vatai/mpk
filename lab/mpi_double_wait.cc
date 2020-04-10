#include <iostream>
#include <mpi.h>

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  std::cout << "Hello, world! This is rankd: " << rank << std::endl;

  int data[2];
  data[0] = rank + rank;
  data[1] = rank * rank;

  MPI_Finalize();
  return 0;
}
