#pragma once

#include <vector>

#include <metis.h>

#include "csr_t.h"

class mpi_comm_data {
  std::vector<int> recvcounts;
  std::vector<int> sendcounts;
  std::vector<int> rdispls;
  std::vector<int> sdispls;
};

class buffer_t {
  std::vector<csr_t> buffers;
  std::vector<std::vector<double>> mval;


};

class buffers_t {
  const idx_t npart;
  buffers_t(const idx_t _npart);
};
