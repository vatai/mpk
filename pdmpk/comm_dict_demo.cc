/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-14
 */

#include <iostream>
#include "comm_dict_t.h"

int main(int argc, char *argv[])
{
  // std::vector<int> a = {1, 2, 3};
  // std::vector<int> b = {10, 20, 30};
  // std::cout << "a: ";
  // for (auto e : a) std::cout << e << ", ";
  // std::cout << "\n";

  // b.insert(std::begin(b) + 1, std::begin(a), std::end(a));
  // std::cout << "b: ";
  // for (auto e : b) std::cout << e << ", ";
  // std::cout << "\n";

  // std::map<int, double> m;
  // m[1] = 10.0;
  // m[10] = 100.0;

  // std::cout << "m:: ";
  // for (auto p : m) std::cout << p.first << ", " << p.second << ";  ";
  // std::cout << "\n";

  // m.clear();

  // std::cout << "m:: ";
  // for (auto p : m) std::cout << p.first << ", " << p.second << ";  ";
  // std::cout << "\n";

  comm_dict_t cdict(3);
  cdict.rec_svert(0, 2, 10, 0);
  cdict.rec_ivert(0, 1, 20, 0);
  cdict.process();
  std::cout << cdict.mpi_bufs << std::endl;
  cdict.clear();
  cdict.process();

  std::cout << cdict.mpi_bufs << std::endl;
  return 0;
}
