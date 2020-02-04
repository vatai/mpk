/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-10-21

#pragma once
#include <iostream>
#include <metis.h>
#include <vector>

#include "args.h"
#include "csr.h"
#include "typedefs.h"

class PDMPKBuffers {
public:
  PDMPKBuffers(const Args &args, const CSR &csr);

  level_t MinLevel() const;
  bool IsFinished() const;
  bool CanAdd(const idx_t &idx, const level_t &lbelow, const idx_t &t) const;
  void IncLevel(const idx_t &idx);
  void UpdateWeights(const level_t &min);

  bool PartialIsFull(const idx_t &idx) const;
  bool PartialIsEmpty(const idx_t &idx) const;
  void PartialReset(const idx_t &idx);
  double PartialCompleted(const idx_t &idx) const;
  /// Since all the vertices are initially at level = 0,
  /// partition is done without any weights input.
  /// METIS_PartGraphKway outputs by updating partitions.data()
  void MetisPartition();
  void MetisPartitionWithWeights();

  void DebugPrintLevels(std::ostream &os);
  void DebugPrintPartials(std::ostream &os);
  void DebugPrintPartitions(std::ostream &os);
  void DebugPrintReport(std::ostream &os, const int &phase);

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
  const Args &args;
  PDMPKBuffers();
};
