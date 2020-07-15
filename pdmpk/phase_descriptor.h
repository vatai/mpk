/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2020-04-19

#include "typedefs.h"
#include <ostream>

/// Phase descriptors, which describes how to process each phase:
/// levels (that is the corresponding batches) are processed for
/// which `bottom <= level and level < top`.
class PhaseDescriptor {
public:
  level_t bottom; ///< Phase and receiving starts at this level.
  level_t top;    ///< Phase stops before this level.

  /// Output operator.
  friend std::ostream &operator<<(std::ostream &os, const PhaseDescriptor &pd);
};
