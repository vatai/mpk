// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2020-04-19

#include "phase_descriptor.h"
#include "typedefs.h"
#include <ostream>

PhaseDescriptor::PhaseDescriptor() : bottom(-1), mid(-1), top(-1) {}

void PhaseDescriptor::Update(const level_t &lbelow) {
  if (bottom == -1) {
    bottom = lbelow;
    mid = lbelow;
  }
  top = top <= lbelow ? lbelow + 1 : top;
}

void PhaseDescriptor::UpdateMid(const level_t &lbelow) {
  if (lbelow < top) {
    mid = mid <= lbelow ? lbelow + 1 : mid;
  }
}

std::ostream &operator<<(std::ostream &os, const PhaseDescriptor &pd) {
  os << "(" << pd.bottom << ", " << pd.mid << ", " << pd.top << ")";
  return os;
}
