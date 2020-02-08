/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-12-22

#pragma once
#include <vector>

/// The `phased_vector` template class implements the `array` +
/// `array_begin` combination where `array_begin[phase]` is the index
/// of the first element of `array` which belongs to the given
/// `phase`.
template <typename T> class phased_vector : public std::vector<T> {
public:
  /// Marks the beginning of a section which belongs to a given phase.
  std::vector<size_t> phase_begin;
  /// Record the beginning of the next phase based on the current size
  /// of the vector.
  void rec_phase_begin() { phase_begin.push_back(this->size()); };
  /// Returns the pointer to the data where the given phase starts.
  ///
  /// @param phase A phase index.
  ///
  /// @returns A pointer to the vector data where the given phase
  /// starts.
  T *get_ptr(const int &phase) { return this->data() + phase_begin[phase]; }
};
