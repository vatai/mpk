/// @author Emil VATAI <emil.vatai@gmail.com>
/// @author Utsav SINGHAL <utsavsinghal5@gmail.com>
/// @date 2019-09-17

/// @brief Communication data for with partial vertices.

#pragma once

#include <map>
#include <string>
#include <vector>

#include <metis.h>

#include "args.h"
#include "buffers.h"
#include "csr.h"
#include "pdmpk_buffers.h"
#include "typedefs.h"

class CommCompPatterns {

public:
  /// Construct and fill all the buffers in a CommCompPatterns object.
  ///
  /// @param [in] args arguments passed to the executable.
  CommCompPatterns(const Args &args);

  /// Print the statistics of communication.
  void Stats() const;

  /// `bufs[part]` contains all the buffers such as `mcsr` and MPI
  /// buffers for each partition `part`.
  std::vector<Buffers> bufs;

private:
  /// Processed arguments provided to the main program.
  const Args &args;

  /// The graph/matrix being processed.
  const Csr csr;

  /// All MPK buffers such as levels, weights, partitions, and
  /// partials.
  PdmpkBuffers pdmpk_bufs;

  /// "Shadow" copy of `pdmpk_bufs` used for optimization.
  PdmpkBuffers pdmpk_count;

  /// (source, target) pair.
  typedef std::pair<idx_t, idx_t> src_tgt_t;

  /// (partition, source index) pair.
  typedef std::pair<idx_t, idx_t> part_sidx_t;

  /// (index, level) pair.
  typedef std::pair<idx_t, level_t> idx_lvl_t;

  /// Map (vector index, level) pair to the (partition, mbuf index)
  /// pair where it can be found.
  typedef std::map<idx_lvl_t, part_sidx_t> StorePart;

  /// @see StorePart
  StorePart store_part;

  /// The type of communication recorded in @ref CommDict @see
  /// comm_dict
  enum CommType {
    kMcol,     ///< Full vertex (needs update `mcol` in @ref Mcsr)
    kInitIdcs, ///< Partial/initialization value (needs update
               ///`init_idcs` in @ref Buffers)
    kFinished  ///< Finished vertex
  };

  /// A structure describing the data needed to be communicated.
  struct SrcType {
    idx_t src_mbuf_idx; ///< The index in `mbuf` (in @ref Buffers) where the
                        /// data can be found, obtained from @ref store_part
    CommType type;      ///< The type of communication @see CommType
    /// Lexical ordering.
    friend bool operator<(const SrcType &lhs, const SrcType &rhs) {
      return std::tie(lhs.src_mbuf_idx, lhs.type) <
             std::tie(rhs.src_mbuf_idx, rhs.type);
    }
  };

  /// For each (mbuf index, type) pair, lists all the indices needed
  /// to be updated (the mbuf index is in the source partition, the
  /// updated indices are on the target partition). @see ProcCommDict
  typedef std::map<SrcType, std::set<idx_t>> Backpatch;

  /// In each phase, collect the communication as a map from (source,
  /// target) pairs to a @ref Backpatch.
  typedef std::map<src_tgt_t, Backpatch> CommDict;
  CommDict comm_dict; ///< @see CommDict.

  /// Communication table used by @ref
  /// CommCompPatterns::NewPartitionLabels.
  typedef std::map<src_tgt_t, std::set<idx_t>> CommTable;
  CommTable comm_table; ///< @see CommTable.

  /// Partition generated by the first unweighted partitioning.
  std::vector<idx_t> home_partition;

  /// Stack of partitions for partition mirroring.
  std::vector<std::vector<idx_t>> partition_history;

  /// Code executed at the end of the constructor.  This method
  ///
  /// - finalizes @ref Mcsr::mptr and @ref MpiBuffers::init_idcs so
  /// their processing can be more convenient;
  ///
  /// - gathers information on how to collect the results.
  void Epilogue();

  /// Process all phases: without any mirroring.
  void ProcAllPhases0();

  /// Process all phases: mirror after `min_level` is above
  /// `nlevel/2`.
  void ProcAllPhases1();

  /// Process all phases: mirror after `min_level` is above 0.
  void ProcAllPhases2();

  /// Process all phases: mirror after `min_level` is above `nlevel/2`
  /// (no `level_sum` check).
  void ProcAllPhases3();

  /// Process all phases: mirror after `min_level` is above 0 (no
  /// `level_sum` check).
  void ProcAllPhases4();

  /// Generate and optimize partition label assignment using @ref
  /// PdmpkBuffers::MetisPartitionWithWeights @ref
  /// CommCompPatterns::OptimizeVertex and @ref
  /// CommCompPatterns::FindLabelPermutation.
  ///
  /// @param min_level Minimum level in the current phase.
  void NewPartitionLabels(const size_t &min_level);

  /// Called in @ref CommCompPatterns::NewPartitionLabels.
  ///
  /// @param idx The index of the vertex processed.
  ///
  /// @param lbelow The level below the current vertex's level
  /// `level[idx]-1`.
  bool OptimizeVertex(const idx_t &idx, const level_t &lbelow);

  /// Called in @ref CommCompPatterns::NewPartitionLabels.
  void FindLabelPermutation();

  /// Code executed before each phase.
  void InitPhase();

  /// Generate one phase.
  void ProcPhase(const size_t &min_level);

  /// Process one vertex.
  ///
  /// @param idx The index of the vertex being processed.
  ///
  /// @param lbelow The current level of the vertex.
  bool ProcVertex(const idx_t &idx, const level_t &lbelow);

  /// Register a partial vertex.
  ///
  /// @param idx The index of the vertex to be registered.
  ///
  /// @param level the level which the vertex tries to achieve
  /// (i.e. `lbelow + 1`).
  void AddToInit(const idx_t &idx, const idx_t &level);

  /// Process adjacent vertex.
  ///
  /// @param idx The index of the vertex being processed/calculated.
  ///
  /// @param lbelow The level of the vertex at `idx`.
  ///
  /// @param col_idx The index of the adjacent vertex in @ref Csr::col.
  void ProcAdjacent(const idx_t &idx, const level_t &lbelow,
                    const idx_t &col_idx);

  /// Clean up after processing a vertex
  ///
  /// @param idx_lvl Index-level pair of the vertex being processed.
  ///
  /// @param part the partition where the vertex can be found can be
  /// found.
  void FinalizeVertex(const idx_lvl_t &idx_lvl, const idx_t &part);

  /// Code executed after each phase.
  void FinalizePhase();

  /// Update the send count on the source partition, and the receive
  /// count on the target partition.
  ///
  /// @param src_tgt_part (Source partition, target partition) pair.
  ///
  /// @param size The size of the by which the given entry should be
  /// increased.
  void UpdateMPICountBuffers(const src_tgt_t &src_tgt_part, const size_t &size);

  /// Process one element/iterator of @ref CommDict. The key of a
  /// CommDict determines the source and target partitions, the @ref
  /// Backpatch determines what needs to be done. The @ref
  /// SrcType::src_mbuf_idx in the index added to @ref Buffers::sbuf,
  /// the @ref SrcType::type determines which what needs to be updated
  /// (@ref Mcsr::mcol for @ref kMcol, @ref MpiBuffers::init_idcs for
  /// @ref kInitIdcs), and the set mapped to by the backpatch contains
  /// all the indices which need to be updated.
  ///
  /// @param iter Iterator representing one element in @ref CommDict.
  void ProcCommDict(const CommDict::const_iterator &iter);

  /// Return the base (0th index) of the subinterval of send buffer in
  /// the source buffer.
  idx_t SrcSendBase(const sidx_tidx_t &src_tgt) const;

  /// Return the base (0th index) of the subinterval of receive buffer
  /// in the target buffer.
  idx_t TgtRecvBase(const sidx_tidx_t &src_tgt) const;

  /// The current phase set at the beginning of each phase.
  int phase;

  //// @todo(vatai): Remove debug DbgPhaseSummary().
  ///
  /// @param min_level Minimum of levels.
  ///
  /// @param level_sum Exact sum of "levels".
  void DbgPhaseSummary(const level_t &min_level, const size_t &level_sum) const;

  /// @todo(vatai): Remove debug DbgAsserts().
  void DbgAsserts() const;

  /// @todo(vatai): Remove debug DbgMbufChecks().
  void DbgMbufChecks() const;

  /// Mirror member function pointer type.
  typedef void (CommCompPatterns::*MirrorFunc)();

  /// Registry of mirror functions. @see CommCompPatterns::MirrorFunc.
  const std::vector<MirrorFunc> mirror_func_registry;
};
