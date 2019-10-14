/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-14
 */

#pragma once

#include <map>
#include <vector>

#include <metis.h>

#include "typedefs.h"
#include "mpi_bufs_t.h"

/**
 * Communication data dictionary.
 *
 * `comm_dict_t` collects communication data in the two dictionaries
 * `sdict` and `idict`.  From that data it is able to create the
 * resulting `mpi_bufs` for the phase.
 */
class comm_dict_t
{
 public:
  /// `npart` is the number of partitions.
  comm_dict_t (const idx_t npart);

  /**
   * Record a complete vertex to `sdict`.
   * @param sp Source Partition id.
   * @param tp Target Partition id.
   * @param ti Target Index in mbuf before processing.
   *
   *
   */
  void rec_svert(const idx_t sp, const idx_t tp, const idx_t si, const idx_t ti);
  /// Record an initialisation vertex to `idict`.
  void rec_ivert(const idx_t sp, const idx_t tp, const idx_t si, const idx_t ti);


  /// Call `process()` to fill the MPI vectors.
  void process();
  /// Clear all the data (call between phases).
  void clear();

  mpi_bufs_t mpi_bufs;

 private:
  comm_dict_t();

  /// The __send__ dictionary for complete vertices.
  std::map<from_to_pair_t, std::vector<idx_t>> sdict;
  /// The dictionary for partial __initialisation__ vertices.
  std::map<from_to_pair_t, idx_t> idict;

};
