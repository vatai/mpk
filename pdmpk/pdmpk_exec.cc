/// @file
/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-11-11

#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <mpi.h>

#include "buffers.h"
#include "utils.hpp"

/// @page pdmpk_exec pdmpk_exec
///
/// The body of `pdmpk_exec` page.

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
  // if (rank == 3) {
  //   int i = 1;
  //   printf("PID %d ready for attach\n", getpid());
  //   fflush(stdout);
  //   while (0 == i)
  //     sleep(5);
  // }

  Buffers buf(npart);
  buf.Load(rank);
  buf.Exec();

  buf.DumpMbufTxt(rank);

  buf.results.Dump(rank);
  buf.results.DumpTxt(rank);

  MPI_Finalize();
  std::cout << "MPI_Finalize()" << std::endl;
  return 0;
}
