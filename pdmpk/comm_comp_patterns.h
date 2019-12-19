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

  /// "Shadow" copy of `pdmpk_bufs` used for optimization.
  PDMPKBuffers pdmpk_count;

  /// Map (vector index, level) pair to the (partition, mbuf index)
  /// pair where it can be found.
  typedef std::map<idx_lvl_t, part_sidx_t> StorePart;

  /// @see StorePart
  StorePart store_part;

  /// The type of communication recorded in @ref CommDict @see
  /// comm_dict
  enum CommType {
    kMcol,    ///< Full vertex (needs update `mcol` in @ref MCSR)
    kInitIdcs ///< Partial/initialization value (needs update
              ///`init_idcs` in @ref Buffers)
  };

  /// A structure describing the data needed to be communicated.
  struct SrcType {
    idx_t src_mbuf_idx; ///< The index in `mbuf` (in @ref Buffers) where the
                        /// data can be found, obtained from @ref store_part
    CommType type;      ///< The type of communication @see CommType
    friend bool operator<(const SrcType &l, const SrcType &r) {
      return std::tie(l.src_mbuf_idx, l.type) <
             std::tie(r.src_mbuf_idx, r.type);
    }
  };

  /// For each (mbuf index, type) pair, lists all the indices needed
  /// to be updated (the mbuf index is in the source partition, the
  /// updated indices are on the target partition). @see ProcCommDict
  typedef std::map<SrcType, std::set<idx_t>> Backpatch;

  /// In each phase, collect the communication as a map from (source,
  /// target) pairs to a @ref Backpatch.
  typedef std::map<src_tgt_t, Backpatch> CommDict;
  /// @see CommDict
  CommDict comm_dict;

  typedef std::map<src_tgt_t, std::set<idx_t>> CommTable;
  CommTable comm_table;

  void OptimizePartitionLabels(size_t min_level);
  bool OptimizeVertex(const idx_t idx, const level_t lbelow);
  void FindLabelPermutation();

  bool ProcPhase(size_t min_level);
  void InitPhase();

  bool ProcVertex(const idx_t idx, const level_t lbelow);
  void AddToInit(const idx_t idx, const idx_t level);
  void ProcAdjacent(const idx_t idx, const level_t lbelow, const idx_t t);
  void FinalizeVertex(const idx_lvl_t idx_lvl, const idx_t part);

  void FinalizePhase();
  void UpdateMPICountBuffers(const src_tgt_t &src_tgt_part, const size_t size);

  /// Process every @ref CommDict. The key of a CommDict determines
  /// the source and target partitions, the @ref Backpatch determines
  /// what needs to be done. The @ref SrcType::src_mbuf_idx in the
  /// index added to @ref Buffers::sbuf, the @ref SrcType::type
  /// determines which what needs to be updated (@ref MCSR::mcol for
  /// @ref kMcol, @ref MPIBuffers::init_idcs for @ref kInitIdcs), and
  /// the set mapped to by the backpatch contains all the indices
  /// which need to be updated.
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
};
