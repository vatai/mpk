/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-11-28

#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <mpi.h>
#include <vector>

#include "buffers_t.h"
#include "utils.hpp"

/// @page pdmpk_test `pdmpk_test`
///
/// Body of `pdmpk_test` page.

int main(int argc, char *argv[])
{
  int rank, npart;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &npart);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  std::vector<double> input;

  std::vector<double> result;
  std::ifstream file("fresult.bin");
  Utils::load_vec(result, file);

  MPI_Finalize();
  std::cout << "MPI_Finalize()" << std::endl;
  return 0;
}
