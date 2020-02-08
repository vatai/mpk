/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-11-12

#pragma once

#include <fstream>
#include <utility>
#include <vector>

namespace Utils {
template <typename T1, typename T2>
/// Print pairs to output stream.
std::ostream &operator<<(std::ostream &os, const std::pair<T1, T2> &pair) {
  os << "(" << pair.first << "," << pair.second << ")";
  return os;
}

/// Dump/save vector to file stream, @see LoadVec.
template <typename T>
void DumpVec(const std::vector<T> &vec, std::ofstream &ofs) {
  const auto size = vec.size();
  ofs.write((char *)&size, sizeof(size));
  ofs.write((char *)vec.data(), sizeof(T) * size);
}

/// Load vector froum file stream, @see DumpVec.
template <typename T> //
void LoadVec(std::istream &is, std::vector<T> *vec) {
  auto size = vec->size();
  is.read((char *)&size, sizeof(size));
  vec->resize(size);
  is.read((char *)vec->data(), sizeof(T) * size);
}

/// Dump/save vector to file stream as text.
template <typename T> //
void DumpTxt(const char *name, const std::vector<T> &vec, std::ofstream &ofs) {
  ofs << name << ": ";
  for (const auto v : vec) {
    ofs << v << ", ";
  }
  ofs << std::endl;
}
} // namespace Utils
