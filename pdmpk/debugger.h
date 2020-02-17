/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2020-02-17

#pragma once

#include "typedefs.h"

class CommCompPatterns;

/// Debugger methods on @ref CommCompPatterns.
class Debugger {
public:
  /// Constructor.
  ///
  /// @param ccp Reference to @ref CommCompPatterns which we intend to
  /// debug.
  Debugger(const CommCompPatterns *const ccp);

  //// @todo(vatai): Remove debug DbgPhaseSummary().
  ///
  /// @param min_level Minimum of levels.
  ///
  /// @param level_sum Exact sum of "levels".
  void PhaseSummary(const level_t &min_level, const size_t &level_sum) const;
  /// @todo(vatai): Remove debug DbgAsserts().
  void Asserts() const;
  /// @todo(vatai): Remove debug DbgMbufChecks().
  void MbufChecks();

private:
  /// Reference to @ref CommCompPatterns which we intend to debug.
  const CommCompPatterns *const ccp;
};
