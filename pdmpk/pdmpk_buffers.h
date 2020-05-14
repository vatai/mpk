/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-10-21

#pragma once
#include <iostream>
#include <metis.h>
#include <vector>

#include "args.h"
#include "csr.h"
#include "typedefs.h"

/// Buffers specific to the PDMPK algorithm.
class PdmpkBuffers {
public:
  /// Constructor.
  ///
  /// @param args Arguments passed to the main program.
  ///
  /// @param csr Matrix/graph read from @ref Args::mtxname.
  PdmpkBuffers(const Args &args, const Csr &csr);

  /// Find the minimum of levels.
  ///
  /// @returns Minumum of @ref PdmpkBuffers::levels.
  level_t MinLevel() const;

  /// Find the exact sum of levels including partial results.
  ///
  /// @returns Sum of @ref PdmpkBuffers::levels + partials.
  size_t ExactLevelSum() const;

  /// Checks if all the algorithm is finished.
  ///
  /// @returns True iff all levels reached @ref Args::nlevel.
  bool IsFinished() const;

  /// Check if an adjacent vertex can be added.
  ///
  /// @param idx Index of the vertex processed.
  ///
  /// @param lbelow Level already obtained by the index processed
  /// (`lbelow + 1` is the "target level" which needs to be
  /// calculated).
  ///
  /// @param t `col` index of the adjacent vertex.
  bool CanAdd(const idx_t &idx, const level_t &lbelow, const idx_t &t) const;

  /// Increase level of a vertex if it is needed.
  ///
  /// @param idx Index of the vertex modified.
  void IncLevel(const idx_t &idx);

  /// Update the edge weights based on the levels by calling the
  /// selected method @see Args.
  ///
  /// @param min Minimum of levels in the current phase.
  void UpdateWeights(const level_t &min);

  /// Check if a vertex is one above the level as specified by @ref
  /// PdmpkBuffers::levels, that is, when the level can be increased.
  ///
  /// @param idx The index of the vertex investigated.
  ///
  /// @return True iff all partial bits of vertex `idx` are 1.
  bool PartialIsFull(const idx_t &idx) const;

  /// Check if a vertex is at the level as specified by @ref
  /// PdmpkBuffers::levels.
  ///
  /// @param idx The index of the vertex investigated.
  ///
  /// @return True iff all partial bits of vertex `idx` are 0.
  bool PartialIsEmpty(const idx_t &idx) const;

  /// Clear (set to 0) all bits of a vertex.
  ///
  /// @param idx Index of the vertex which is to be cleared.
  void PartialReset(const idx_t &idx);

  /// Calculate the amount a partial vertex is computed: computed
  /// neighbours/all neighbours.
  ///
  /// @param idx The index investigated.
  ///
  /// @return A double with value between 0 and 1.
  double PartialCompleted(const idx_t &idx) const;

  /// Since all the vertices are initially at level = 0,
  /// partition is done without any weights input.
  /// METIS_PartGraphKway outputs by updating partitions.data()
  void MetisPartition();

  /// Repartition the graph/matrix into partitions using @ref
  /// PdmpkBuffers::weights.
  void MetisPartitionWithWeights();

  /// Fix Metis partitioning of `idx`.
  ///
  /// @param idx The index of the vertex which needs fixing.
  void MetisFixVertex(idx_t idx);

  /// Function called in @ref DebugPrintReport.
  void DebugPrintLevels(std::ostream &os);
  /// Function called in @ref DebugPrintReport.
  void DebugPrintPartials(std::ostream &os);
  /// Function called in @ref DebugPrintReport.
  void DebugPrintPartitions(std::ostream &os);
  /// Debug function to print a summary/report for one phase.
  void DebugPrintReport(std::ostream &os, const int &phase);

  /// `partials` stores the availibility of adjacent vertex's
  /// value to our target vertex(True if available)
  std::vector<bool> partials;
  /// Stores partitions of the phase allotted by Metis
  std::vector<idx_t> partitions;
  /// Stores the levels of vertices just before starting of phase
  std::vector<level_t> levels;

private:
  /// Edge weights used for partitioning  by Metis.
  std::vector<idx_t> weights;

  /// Matrix/graph @see Csr.
  const Csr &csr;

  /// Arguments passed to the main program.
  const Args &args;

  /// Weights update member function pointer type.
  typedef void (PdmpkBuffers::*UpdateWeightsFunc)(const level_t &);

  /// Registry of update functions. @see
  /// PdmpkBuffers::UpdateWeightsFunc.
  const std::vector<UpdateWeightsFunc> update_func_registry;

  /// Pointer to the default UpdateWeights function. @see
  /// PdmpkBuffers::UpdateWeightsFunc.
  const UpdateWeightsFunc update_weights_func;

  /// Disabled default constructor.
  PdmpkBuffers();

  /// The exact level of vertex (including partial results).
  ///
  /// @param idx Index of the vertex.
  ///
  /// @returns |Adj(idx)| * levels[idx] + sum(partials[idx]).
  size_t ExactLevel(idx_t idx) const;

  /// The original weight update function `1/(li + lj - 2*min + 1)`,
  /// where `li = levels[i]` and `lj = levels[j]`. @see
  /// PdmpkBuffers::UpdateWeightsFunc.
  ///
  /// @param min Minimum of levels in the current phase.
  void UpdateWeights0(const level_t &min);

  /// The weight update function, which uses the partials.  @see
  /// PdmpkBuffers::UpdateWeightsFunc.
  ///
  /// @param min Minimum of levels in the current phase.
  void UpdateWeights1(const level_t &min);

  /// The weight update function, which uses the partials.  @see
  /// PdmpkBuffers::UpdateWeightsFunc.
  ///
  /// @param min Minimum of levels in the current phase.
  void UpdateWeights2(const level_t &min);

  /// The weight update function, which uses the partials.  @see
  /// PdmpkBuffers::UpdateWeightsFunc.
  ///
  /// @param min Minimum of levels in the current phase.
  void UpdateWeights3(const level_t &min);
};
