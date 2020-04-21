/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2020-04-19

#include "typedefs.h"
#include <ostream>

/// Phase descriptors, which describes how to process each phase:
/// levels (that is the corresponding batches) are processed for
/// which `bottom <= level and level < top`, but only levels `bottom
/// <= level < mid` are checking if the data arrived.
class PhaseDescriptor {
public:
  level_t bottom; ///< Phase and receiving starts at this level.
  level_t mid;    ///< Receiving stops before this level.
  level_t top;    ///< Phase stops before this level.

  /// Constructor.
  PhaseDescriptor();

  /// Update `bottom` and `top` members: `bottom` becomes `lbelow` if
  /// `bottom` was 0, `top` becomes `max(top, lbelow + 1)`.
  ///
  /// @param lbelow Level of the batch processed.
  void Update(const level_t &lbelow);

  /// Update mid: set it to `max(lbelow + 1, mid)`.
  ///
  /// @param lbelow Level of the batch processed.
  void UpdateMid(const level_t &lbelow);

  /// Output operator.
  friend std::ostream &operator<<(std::ostream &os, const PhaseDescriptor &pd);
};
