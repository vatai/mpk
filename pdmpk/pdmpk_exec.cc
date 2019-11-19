/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-11-11

#include <fstream>
#include <iostream>
#include <unistd.h>
#include <mpi.h>

#include "buffers_t.h"
#include "utils.hpp"

int main(int argc, char *argv[])
{
  int rank, npart;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &npart);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // MPI debuging:
  // - start with: $ gdb pdmpk_exec PID
  // - up # go up in the stack
  // - up # two times
  // - set var i = 1
  // - continue
  if (rank == 0) {
    int i = 0;
    printf("PID %d ready for attach\n", getpid());
    fflush(stdout);
    while (0 == i)
      sleep(5);
  }

  buffers_t buf(npart);
  buf.load(rank);
  buf.exec();

  std::string fname = "dresult" + std::to_string(rank) + ".txt";
  std::ofstream file(fname);
  Utils::dump_txt("mbuf", buf.mbuf, file);
  file.close(); /// @todo(vatai): Write proper results test.
  MPI_Finalize();
  std::cout << "MPI_Finalize()" << std::endl;
  return 0;
}
