// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2020-04-19

#include "phase_descriptor.h"
#include "typedefs.h"
#include <ostream>


std::ostream &operator<<(std::ostream &os, const PhaseDescriptor &pd) {
  os << "(" << pd.bottom <<  ", " << pd.top << ")";
  return os;
}
