#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <mpi.h>
#include "lib.h"


int main(int argc, char* argv[]){
  MPI_Init(&argc, &argv);
  int world_rank, world_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv[0]);
    exit(1);
  }

  mpk_t *mg = read_mpk(argv[1]); // *******

  int n = mg->n;
  int nlevel = mg->nlevel;

  // vv stores the vector (vertices) at different levels
  double *vv = (double*) malloc(sizeof(double) * n * (nlevel+1));
  assert(vv != NULL);

  printf("world_rank: %d;\n", world_rank);
  printf("argc: %d;\n", argc);
  printf("argv[0]: %s\n", argv[0]);
  printf("argv[1]: %s\n", argv[1]);

  MPI_Finalize();
  return 0;
}
