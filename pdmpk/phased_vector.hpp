#pragma once
#include <vector>

/// The `phased_vector` template class implements the `array` +
/// `array_begin` combination where `array_begin[phase]` is the index
/// of the first element of `array` which belongs to the given
/// `phase`.
template <typename T> class phased_vector : public std::vector<T> {
 public:
  std::vector<size_t> begin;
  void rec_begin() { begin.push_back(this->size()); };
};
