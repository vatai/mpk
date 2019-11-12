/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-11-12

#pragma once

#include <fstream>
#include <vector>

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
