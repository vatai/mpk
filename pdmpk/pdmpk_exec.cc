/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-11-11

#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <mpi.h>

#include "buffers_t.h"
#include "utils.hpp"

/**
 * @page pdmpk_exec_page `pdmpk_exec` page
 * The body of pdmpk_exec_page.
 */
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
  if (rank == 3) {
    int i = 1;
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

  /// @todo(vatai): Write proper result collection.
  fname = "fresult" + std::to_string(rank) + ".txt";
  file.open(fname);
  const auto size = buf.result_idx.size();
  std::vector<double> fresults(size);
  for (size_t i = 0; i < size; i++) {
    fresults[i] = buf.mbuf[buf.result_idx[i]];
  }
  Utils::dump_txt("final:", fresults, file);
  file.close();

  MPI_Finalize();
  std::cout << "MPI_Finalize()" << std::endl;
  return 0;
}
