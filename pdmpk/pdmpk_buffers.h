/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-10-21

#pragma once
#include <iostream>
#include <metis.h>
#include <vector>

#include "csr.h"
#include "typedefs.h"

class PDMPKBuffers {
public:
  PDMPKBuffers(const CSR &csr);

  level_t MinLevel();
  bool CanAdd(const idx_t idx, const level_t lbelow, const idx_t t);
  void IncLevel(const idx_t idx);
  void UpdateWeights();

  bool PartialIsFull(const idx_t idx) const;
  bool PartialIsEmpty(const idx_t idx) const;
  void PartialReset(const idx_t idx);
  /// Since all the vertices are initially at level = 0,
  /// partition is done without any weights input.
  /// METIS_PartGraphKway outputs by updating partitions.data()
  void MetisPartition(idx_t npart);
  void MetisPartitionWithWeights(idx_t npart);

  void DebugPrintLevels(std::ostream &os);
  void DebugPrintPartials(std::ostream &os);
  void DebugPrintPartitions(std::ostream &os);
  void DebugPrintReport(std::ostream &os, const int phase);

  /// `partials` stores the availibility of adjacent vertex's
  /// value to our target vertex(True if available)
  std::vector<bool> partials;
  /// Stores partitions of the phase allotted by Metis
  std::vector<idx_t> partitions;
  /// Stores the levels of vertices just before starting of phase
  std::vector<level_t> levels;

private:
  std::vector<idx_t> weights;
  const CSR &csr;
  PDMPKBuffers();
};
