/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-09-17
 *
 * @mainpage Partial Diamon Matrix Powers Kernel
 *
 * Given a (sparse) matrix \f$A \in \mathbb{R}^{n \times n}\f$ and
 *   vector \f$v \in \mathbb{R}\f$ MPK generates \f[ Av, A^2v, \ldots,
 *   A^m v \f]
 *
 * This program generates the communication pattern for a given matrix
 * \f$A\f$.
 *
 * @todo(vatai) Finish documentation here.
 * @todo(vatai): Remove rank if not needed.
 * @todo(vatai): Implement MPI class if needed.
 */

#include <iostream>
#include <sstream>
#include <mpi.h>

#include "typedefs.h"
#include "partial_cd.h"

int main(int argc, char *argv[])
{
  int rank = 0;
  idx_t npart;
  level_t nlevels;

  // MPI_Init(&argc, &argv);
  // MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Get npart
  std::stringstream npart_ss(argv[2]);
  npart_ss >> npart;
  std::stringstream nlevels_ss(argv[3]);
  nlevels_ss >> nlevels;

  if (rank == 0) {
    partial_cd pcd(argv[1], rank, npart, nlevels);
  }

  // MPI_Finalize();
  return 0;
}
