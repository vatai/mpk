/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-11-11

#include <iostream>
#include <mpi.h>

#include "buffers_t.h"

int main(int argc, char *argv[])
{
  int rank, npart;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &npart);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  buffers_t buf(npart);
  buf.load(rank);
  std::cout << "mbuf_idx" << buf.mbuf_idx
            << " @ " << rank << std::endl;

  /// @todo(vatai): Implement computation.
  buf.exec();
  MPI_Finalize();
  return 0;
}
