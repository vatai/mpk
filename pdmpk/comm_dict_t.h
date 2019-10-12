#pragma once

#include <map>
#include <vector>

#include <metis.h>

#include "typedefs.h"
#include "mpi_bufs_t.h"

/**
 * @brief Communication data dictionary.
 *
 * `comm_dict_t` collects communication data in the two dictionaries
 * `sdict` and `idict`.  From that data it is able to create the
 * resulting `mpi_bufs` for the phase.
 */
class comm_dict_t
{
 public:
  /// @brief `npart` is the number of partitions.
  comm_dict_t (const idx_t npart);

  /// @brief Record a complete vertex to `sdict`.
  void rec_svert(const idx_t from, const idx_t to, const idx_t idx);
  /// @brief Record an initialisation vertex to `idict`.
  void rec_ivert(const idx_t from, const idx_t to, const idx_t idx);


  /// @brief Call `process()` to fill the MPI vectors.
  void process();
  /// @brief Clear all the data (call between phases).
  void clear();

  mpi_bufs_t mpi_bufs;

 private:
  comm_dict_t();

  /// @brief The __send__ dictionary for complete vertices.
  std::map<from_to_pair_t, std::vector<idx_t>> sdict;
  /// @brief The dictionary for partial __initialisation__ vertices.
  std::map<from_to_pair_t, idx_t> idict;

};
