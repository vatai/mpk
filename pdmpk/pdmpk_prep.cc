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
  idx_t npart;
  level_t nlevels;

  // Get npart
  std::stringstream npart_ss(argv[2]);
  npart_ss >> npart;
  std::stringstream nlevels_ss(argv[3]);
  nlevels_ss >> nlevels;

  partial_cd pcd(argv[1], npart, nlevels);

  return 0;
}
