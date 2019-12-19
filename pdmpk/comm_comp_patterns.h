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
  /// Construct and fill all the buffers in a CommCompPatterns object.
  CommCompPatterns(const char *fname, const idx_t npart, const level_t nlevels);
  /// Print the statistics of communication.
  void Stats(const std::string &name);

  /// `bufs[part]` contains all the buffers such as `mcsr` and MPI
  /// buffers for each partition `part`.
  std::vector<Buffers> bufs;

private:
  enum CommType { kMcol, kInitIdcs };

  struct SrcTgtType {
    idx_t src_mbuf_idx;
    idx_t tgt_idx;
    CommType type;
    friend bool operator<(const SrcTgtType &l, const SrcTgtType &r) {
      return std::tie(l.src_mbuf_idx, l.tgt_idx, l.type) <
             std::tie(r.src_mbuf_idx, r.tgt_idx, r.type);
    }
  };
  /// The graph/matrix being processed.
  const CSR csr;
  /// Number of partition/processes.
  const idx_t npart;
  /// Number of levels the algorithm aims to achieve.
  const level_t nlevels;

  /// All MPK buffers such as levels, weights, partitions, and
  /// partials.
  PDMPKBuffers pdmpk_bufs;

  /// Map (vector index, level) pair to the (partition, mbuf index)
  /// pair where it can be found.
  StorePart store_part;

  /// In each phase, collect the communication of complete indices as
  /// a map from (source, target) pairs to `mbuf` indices of the
  /// source partition. (In case of type = kMcol) Also, collect the
  /// communication of partial indices (for initialization) as a map
  /// from (source, target) pairs to (mbuf indices of source
  /// partition, mcol indices in the target partition) pairs. (In case
  /// of type = kInitIdcs)
  typedef std::map<src_tgt_t, std::set<SrcTgtType>> CommDict;
  CommDict comm_dict;

  /// Code executed before each phase.
  void InitPhase();
  /// Generate one phase.
  bool ProcPhase();

  /// Process one vertex.
  ///
  /// @param idx The index of the vertex being processed.
  ///
  /// @param lbelow The current level of the vertex.
  bool ProcVertex(const idx_t idx, const level_t lbelow);
  /// Register a partial vertex.
  ///
  /// @param idx The index of the vertex to be registered.
  ///
  /// @param level the level which the vertex tries to achieve
  /// (i.e. `lbelow + 1`).
  void AddToInit(const idx_t idx, const idx_t level);
  /// Process adjacent vertex.
  ///
  /// @param idx The index of the vertex being processed/calculated.
  ///
  /// @param lbelow The level of the vertex at `idx`.
  ///
  /// @param t The index of the adjacent vertex in @ref Buffers::mbuf.
  void ProcAdjacent(const idx_t idx, const level_t lbelow, const idx_t t);
  /// Clean up after processing a vertex
  ///
  /// @param idx_lvl Index-level pair of the vertex being processed.
  ///
  /// @param part the partition where the vertex can be found can be
  /// found.
  void FinalizeVertex(const idx_lvl_t idx_lvl, const idx_t part);

  /// Code executed after each phase.
  void FinalizePhase();
  /// Update the send count on the source partition, and the receive
  /// count on the target partition.
  ///
  /// @param src_tgt_part (Source partition, target partition) pair.
  ///
  /// @param size The size of the by which the given entry should be
  /// increased.
  void UpdateMPICountBuffers(const src_tgt_t &src_tgt_part, const size_t size);
  /// Process one element in the @ref CommDict.
  ///
  /// @param iter Iterator representing one element in @ref CommDict.
  void ProcCommDict(const CommDict::const_iterator &iter);

  /// Return the base (0th index) of the subinterval of send buffer in
  /// the source buffer.
  idx_t SrcSendBase(const sidx_tidx_t src_tgt) const;

  /// Return the base (0th index) of the subinterval of receive buffer
  /// in the target buffer.
  idx_t TgtRecvBase(const sidx_tidx_t src_tgt) const;

  /// The current phase set at the beginning of each phase.
  int phase;

#ifndef NDEBUG
  /// @todo(vatai): Remove debug DbgAsserts().
  void DbgAsserts() const;
  /// @todo(vatai): Remove debug DbgMbufChecks().
  void DbgMbufChecks();
#endif
};
