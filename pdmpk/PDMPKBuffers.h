/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-10-21

#pragma once
#include <iostream>
#include <metis.h>
#include <vector>

#include "CSR.h"
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

  void MetisPartition(idx_t npart);
  void MetisPartitionWithLevels(idx_t npart);

  void DebugPrintLevels(std::ostream &os);
  void DebugPrintPartials(std::ostream &os);
  void DebugPrintPartitions(std::ostream &os);
  void DebugPrintReport(std::ostream &os, const int phase);

  std::vector<bool> partials;
  std::vector<idx_t> partitions;
  std::vector<level_t> levels;

private:
  std::vector<idx_t> weights;
  const CSR &csr;
  PDMPKBuffers();
};
