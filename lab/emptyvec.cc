#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

int main(int argc, char *argv[]) {
  std::map<int, std::vector<int>> map;
  map[43];
  std::cout << "map.size(): " << map.size() << std::endl;
  const auto iter1 = map.find(42);
  const auto iter2 = map.find(43);
  if (iter1 != std::end(map))
    std::cout << "42 found" << std::endl;
  else
    std::cout << "42 not found" << std::endl;
  if (iter2 != std::end(map))
    std::cout << "43 found" << std::endl;
  else
    std::cout << "43 not found" << std::endl;
  return 0;
}
