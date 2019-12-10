#pragma once
#include <vector>

/// The `phased_vector` template class implements the `array` +
/// `array_begin` combination where `array_begin[phase]` is the index
/// of the first element of `array` which belongs to the given
/// `phase`.
template <typename T> class phased_vector : public std::vector<T> {
 public:
  std::vector<size_t> phase_begin;
  void rec_phase_begin() { phase_begin.push_back(this->size()); };
  T *get_ptr(const int phase) { return this->data() + phase_begin[phase]; }
};
