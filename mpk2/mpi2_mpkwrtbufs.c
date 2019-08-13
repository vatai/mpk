#include <stdlib.h>
#include <mpi.h>

#include "buffers.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv[0]);
    exit(1);
  }

  // Init MPI
  MPI_Init(&argc, &argv);

  comm_data_t *cd = new_comm_data(argv[1]);
  buffers_t *bufs = new_bufs(cd);

  fill_buffers(cd, bufs);
  write_buffers(bufs, argv[1]);

  del_bufs(bufs);
  del_comm_data(cd);
  MPI_Finalize();
  return 0;
}
