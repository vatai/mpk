/// @author Emil VATAI <emil.vatai@gmail.com>
/// @author Utsav SINGHAL <utsavsinghal5@gmail.com>
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
  /// buffers for each partition `part`.
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
  typedef std::map<idx_lvl_t, part_sidx_t> StorePart;
  StorePart store_part;

  /// In each phase, collect the communication of complete indices as
  /// a map from (source, target) pairs to `mbuf` indices of the
  /// source partition. (In case of type = kMcol)
  /// Also, collect the communication of partial indices (for
  /// initialization) as a map from (source, target) pairs to (mbuf
  /// indices of source partition, mcol indices in the target
  /// partition) pairs. (In case of type = kInitIdcs)
  enum CommType { kMcol, kInitIdcs };
  struct SrcType {
    idx_t src_mbuf_idx;
    CommType type;
    friend bool operator<(const SrcType &l, const SrcType &r) {
      return std::tie(l.src_mbuf_idx, l.type) <
             std::tie(r.src_mbuf_idx, r.type);
    }
  };
  typedef std::map<src_tgt_t, std::map<SrcType, std::set<idx_t>>> CommDict;
  CommDict comm_dict;

  typedef std::map<src_tgt_t, std::set<idx_t>> CommTable;
  CommTable comm_table;

  void OptimizePartitionLabels();
  bool OptimizeVertex(const idx_t idx, const level_t lbelow,
                      PDMPKBuffers *pdmpk_bufs);

  bool ProcPhase();
  void InitPhase();

  bool ProcVertex(const idx_t idx, const level_t lbelow);
  void AddToInit(const idx_t idx, const idx_t level);
  void ProcAdjacent(const idx_t idx, const level_t lbelow, const idx_t t);
  void FinalizeVertex(const idx_lvl_t idx_lvl, const idx_t part);

  void FinalizePhase();
  void UpdateMPICountBuffers(const src_tgt_t &src_tgt_part, const size_t size);
  void ProcCommDict(const CommDict::const_iterator &iter);

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
  void DbgCountTable();
};
