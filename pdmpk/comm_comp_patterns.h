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
  ///
  /// @param mtxname The filename of the mtx file.
  ///
  /// @param npart @see npart
  ///
  /// @param nlevels @see nlevels
  CommCompPatterns(const std::string &mtxname, //
                   const idx_t npart,          //
                   const level_t nlevels);
  /// Print the statistics of communication.
  void Stats(const std::string &name);

  /// `bufs[part]` contains all the buffers such as `mcsr` and MPI
  /// buffers for each partition `part`.
  std::vector<Buffers> bufs;

private:
  /// The graph/matrix being processed.
  const CSR csr;
  /// Number of partition/processes.
  const idx_t npart;
  /// Number of levels the algorithm aims to achieve.
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

  /// Code executed before each phase.
  void InitPhase();
  /// Generate one phase.
  bool ProcPhase(size_t min_level);

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

  /// Process one element/iterator of @ref CommDict. The key of a
  /// CommDict determines the source and target partitions, the @ref
  /// Backpatch determines what needs to be done. The @ref
  /// SrcType::src_mbuf_idx in the index added to @ref Buffers::sbuf,
  /// the @ref SrcType::type determines which what needs to be updated
  /// (@ref MCSR::mcol for @ref kMcol, @ref MPIBuffers::init_idcs for
  /// @ref kInitIdcs), and the set mapped to by the backpatch contains
  /// all the indices which need to be updated.
  ///
  /// @param iter Iterator representing one element in @ref CommDict.
  void ProcCommDict(const CommDict::const_iterator &iter);

  /// Return the base (0th index) of the subinterval of send buffer in
  /// the source buffer.
  idx_t SrcSendBase(const sidx_tidx_t src_tgt) const;

  /// Return the base (0th index) of the subinterval of receive buffer
  /// in the target buffer.
  idx_t TgtRecvBase(const sidx_tidx_t src_tgt) const;

  /// Name of the graph (usually the mtx filename without the
  /// extension).
  const std::string mtxname;

  /// The current phase set at the beginning of each phase.
  int phase;

#ifndef NDEBUG
  /// @todo(vatai): Remove debug DbgAsserts().
  void DbgAsserts() const;
  /// @todo(vatai): Remove debug DbgMbufChecks().
  void DbgMbufChecks();
#endif
};
