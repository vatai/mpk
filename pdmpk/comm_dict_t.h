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

  /// @brief Record a complete vertex to `sdict`.
  void rec_svert(const idx_t from, const idx_t to, const idx_t idx);
  /// @brief Record an initialisation vertex to `idict`.
  void rec_ivert(const idx_t from, const idx_t to, const idx_t idx);

  /// @brief Output the contents of the MPI buffers.
  void serialise(std::ostream &os);

  /// @brief The __send__ dictionary for complete vertices.
  std::map<from_to_pair_t, std::vector<idx_t>> sdict;
  /// @brief The dictionary for partial (initialisation) vertices.
  std::map<from_to_pair_t, idx_t> idict;


  /// @brief Call `process()` to fill the MPI vectors.
  void process();
  /// @brief Clear all the data (call between phases).
  void clear();

  std::vector<std::vector<int>> recvbuf;
  std::vector<std::vector<int>> sendbuf;
  std::vector<std::vector<int>> recvcount;
  std::vector<std::vector<int>> sendcount;
  std::vector<std::vector<int>> recvdispl;
  std::vector<std::vector<int>> senddispl;

 private:
  idx_t npart;
  comm_dict_t();
};
