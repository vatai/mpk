#pragma once

#include <iostream>
#include <map>
#include <vector>

#include <metis.h>

#include "typedefs.h"

class comm_dict_t
{
 public:
  comm_dict_t (const idx_t npart);

  void record(const idx_t from, const idx_t to, const idx_t idx);
  std::vector<idx_t> &view(const idx_t from, const idx_t to);
  void serialise(std::ostream &os);

  /// @brief The __send__ dictionary for complete vertices.
  std::map<from_to_pair_t, std::vector<idx_t>> sdict;
  /// @brief The dictionary for partial (initialisation) vertices.
  std::map<from_to_pair_t, idx_t> idict;


  // Call this before accessing the MPI vectors.
  void process();

  void clear();

 private:
  idx_t npart;
  comm_dict_t();
  std::vector<std::vector<int>> recvbuf;
  std::vector<std::vector<int>> sendbuf;
  std::vector<std::vector<int>> recvcount;
  std::vector<std::vector<int>> sendcount;
  std::vector<std::vector<int>> recvdispl;
  std::vector<std::vector<int>> senddispl;
};
