// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2020-04-19

#include "phase_descriptor.h"
#include "typedefs.h"
#include <ostream>

PhaseDescriptor::PhaseDescriptor() : bottom(-1), mid(0), top(0) {}

void PhaseDescriptor::UpdateBottom(const level_t &lbelow) {
  bottom = bottom == -1 ? lbelow : bottom;
}

void PhaseDescriptor::UpdateMid(const level_t &lbelow) {
  mid = mid <= lbelow ? lbelow + 1 : mid;
}

void PhaseDescriptor::UpdateTop(const level_t &lbelow) {
  top = top <= lbelow ? lbelow + 1 : top;
}

std::ostream &operator<<(std::ostream &os, const PhaseDescriptor &pd) {
  os << "(" << pd.bottom << ", " << pd.mid << ", " << pd.top << "), ";
  return os;
}
