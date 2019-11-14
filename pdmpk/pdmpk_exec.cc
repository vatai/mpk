/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-11-11

#include <fstream>
#include <iostream>
#include <mpi.h>

#include "buffers_t.h"
#include "utils.hpp"

int main(int argc, char *argv[])
{
  int rank, npart;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &npart);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  buffers_t buf(npart);
  buf.load(rank);
  buf.exec();

  std::string fname = "dresult" + std::to_string(rank) + ".txt";
  std::ofstream file(fname);
  Utils::dump_txt("mbuf", buf.mbuf, file);
  file.close(); /// @todo(vatai): Write proper results test.
  MPI_Finalize();
  return 0;
}
