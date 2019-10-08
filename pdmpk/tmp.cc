#include <iostream>
#include "comm_dict_t.h"

int main(int argc, char *argv[])
{
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {10, 20, 30};
  std::cout << "a: ";
  for (auto e : a) std::cout << e << ", ";
  std::cout << "\n";

  b.insert(std::begin(b) + 1, std::begin(a), std::end(a));
  std::cout << "b: ";
  for (auto e : b) std::cout << e << ", ";
  std::cout << "\n";

  comm_dict_t cdict(3);
  cdict.record(0, 2, 10);
  cdict.record(0, 1, 20);
  cdict.process();
  cdict.serialise(std::cout);
  return 0;
}
