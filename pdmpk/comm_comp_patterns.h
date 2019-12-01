/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-09-17

/// @brief Communication data for with partial vertices.

#pragma once

#include <map>
#include <string>
#include <vector>

#include <metis.h>

#include "buffers.h"
#include "csr.h"
#include "pdmpk_buffers.h"
#include "typedefs.h"

class CommCompPatterns {

public:
  CommCompPatterns(const char *fname, const idx_t npart, const level_t nlevels);

  const CSR csr;
  const idx_t npart;
  const level_t nlevels;

  /// All MPK buffers such as levels, weights, partitions, and
  /// partials.
  PDMPKBuffers pdmpk_bufs;

  /// `bufs[part]` is constins all the buffers such as `mcsr` and MPI
  /// buffers for partition `part`.
  std::vector<Buffers> bufs;

private:
  /// Map (vector index, level) pair to the (partition, mbuf index)
  /// pair where it is can be found.
  store_part_t store_part;

  /// In each phase, collect the communication of complete indices as
  /// a map from (source, target) pairs to `mbuf` indices of the
  /// source partition.
  comm_dict_t comm_dict;

  /// In each phase, collect the communication of partial indices (for
  /// initialization) as a map from (source, target) pairs to (mbuf
  /// indices of source partition, mcol indices in the target
  /// partition) pairs.
  init_dict_t init_dict;

  void PhaseInit();

  bool UpdateLevels();
  bool ProcVertex(const idx_t idx, const level_t lbelow);
  void AddToInit(const idx_t idx, const idx_t level);
  void ProcAdjacent(const idx_t idx, const level_t lbelow, const idx_t t);
  void FinalizeVertex(const idx_lvl_t idx_lvl, const idx_t part);

  void PhaseFinalize();
  void ProcCommDict(const comm_dict_t::const_iterator &iter);
  void ProcInitDict(const init_dict_t::const_iterator &iter);

  /// `src_send_base(src, tgt)` gives the base (0th index) of the send
  /// buffer in the source buffer.
  idx_t SrcSendBase(const sidx_tidx_t src_tgt) const;

  /// `tgt_recv_base(src, tgt)` gives the base (0th index) of the
  /// receive buffer in the target buffer.
  idx_t TgtRecvBase(const sidx_tidx_t src_tgt) const;

  /// The current phase is set at the beginning of each phase.
  int phase;

  // ////// DEBUG //////
  void DbgAsserts() const;
  void DbgMbufChecks();
};
