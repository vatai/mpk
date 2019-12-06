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

  /// `bufs[part]` contains all the buffers such as `mcsr` and MPI
  /// buffers for partition `part`.
  std::vector<Buffers> bufs;

private:
  const CSR csr;
  const idx_t npart;
  const level_t nlevels;

  /// All MPK buffers such as levels, weights, partitions, and
  /// partials.
  PDMPKBuffers pdmpk_bufs;

  /// Map (vector index, level) pair to the (partition, mbuf index)
  /// pair where it can be found.
  StorePart store_part;

  /// In each phase, collect the communication of complete indices as
  /// a map from (source, target) pairs to `mbuf` indices of the
  /// source partition.
  CommDict comm_dict;

  /// In each phase, collect the communication of partial indices (for
  /// initialization) as a map from (source, target) pairs to (mbuf
  /// indices of source partition, mcol indices in the target
  /// partition) pairs.
  InitDict init_dict;

  void InitPhase();
  bool ProcPhase();

  bool ProcVertex(const idx_t idx, const level_t lbelow);
  void AddToInit(const idx_t idx, const idx_t level);
  void ProcAdjacent(const idx_t idx, const level_t lbelow, const idx_t t);
  void FinalizeVertex(const idx_lvl_t idx_lvl, const idx_t part);

  void FinalizePhase();
  void UpdateMPICountBuffers(const src_tgt_t &src_tgt_part, const size_t size);
  void ProcCommDict(const CommDict::const_iterator &iter);
  void ProcInitDict(const InitDict::const_iterator &iter);

  /// Return the base (0th index) of the subinterval of send buffer in
  /// the source buffer.
  idx_t SrcSendBase(const sidx_tidx_t src_tgt) const;

  /// Return the base (0th index) of the subinterval of receive buffer
  /// in the target buffer.
  idx_t TgtRecvBase(const sidx_tidx_t src_tgt) const;

  /// The current phase set at the beginning of each phase.
  int phase;

  // ////// DEBUG //////
  void DbgAsserts() const;
  void DbgMbufChecks();
};
