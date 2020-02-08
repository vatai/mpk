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
class PDMPKBuffers {
public:
  /// Constructor.
  ///
  /// @param args Arguments passed to the main program.
  ///
  /// @param csr Matrix/graph read from @ref Args::mtxname.
  PDMPKBuffers(const Args &args, const CSR &csr);

  /// Find the minimum of levels.
  ///
  /// @returns Minumum of @ref PDMPKBuffers::levels.
  level_t MinLevel() const;
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
  /// Update the edge weights based on the levels.
  ///
  /// @param min Minimum of levels in the current phase.
  void UpdateWeights(const level_t &min);

  /// Check if a vertex is one above the level as specified by @ref
  /// PDMPKBuffers::levels, that is, when the level can be increased.
  ///
  /// @param idx The index of the vertex investigated.
  ///
  /// @return True iff all partial bits of vertex `idx` are 1.
  bool PartialIsFull(const idx_t &idx) const;
  /// Check if a vertex is at the level as specified by @ref
  /// PDMPKBuffers::levels.
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
  /// PDMPKBuffers::weights.
  void MetisPartitionWithWeights();

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
  std::vector<idx_t> weights; ///< Edge weights used for partitioning
                              /// by Metis.
  const CSR &csr;             ///< Matrix/graph @see CSR.
  const Args &args;           ///< Arguments passed to the main program.
  PDMPKBuffers();             ///< Disabled default constructor.
};
