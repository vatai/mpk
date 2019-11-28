/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-11-12

#pragma once

#include <fstream>
#include <utility>
#include <vector>

namespace Utils {
template <typename T1, typename T2>
std::ostream &operator<<(std::ostream &os, const std::pair<T1, T2> &pair) {
  os << "(" << pair.first << "," << pair.second << ")";
  return os;
}

template <typename T>
void dump_vec(const std::vector<T> &vec, std::ofstream &ofs) {
  const auto size = vec.size();
  ofs.write((char *)&size, sizeof(size));
  ofs.write((char *)vec.data(), sizeof(T) * size);
}

template <typename T> //
void load_vec(std::vector<T> &vec, std::istream &is) {
  auto size = vec.size();
  is.read((char *)&size, sizeof(size));
  vec.resize(size);
  is.read((char *)vec.data(), sizeof(T) * size);
}

template <typename T> //
void dump_txt(const char *name, std::vector<T> &vec, std::ofstream &ofs) {
  ofs << name << ": ";
  for (const auto v : vec) {
    ofs << v << ", ";
  }
  ofs << std::endl;
}
} // namespace Utils
