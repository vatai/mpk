#include <mpi.h>

#include "partial_cd.h"

int main(int argc, char* argv[]) {
  // check_args(argc, argv[0]);
  MPI_Init(&argc, &argv);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  struct partial_cd *pcd = new_partial_cd(argv[1], rank);
  // buffers_t *bufs = new_bufs(cd);
  // fill_buffers(cd, bufs);

  // double *val = alloc_read_val(cd->graph, argv[1]);
  // alloc_fill_mval(bufs, val, cd->graph);

  // write_buffers(bufs, argv[1]);

  // del_bufs(bufs);
  // del_comm_data(cd);
  MPI_Finalize();
  return 0;
}
