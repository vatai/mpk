#include <algorithm>
#include <vector>
#include <iostream>

int main(int argc, char *argv[])
{
  std::vector<int> vec {1, 42, 2, 3, 1, 1, 2};

  for (int i = 0; i < vec.size(); i++)
    std::cout << i << ": " << vec[i] << std::endl;

  auto riter = std::find(rbegin(vec), rend(vec), 2);
  auto rdiff = rend(vec) - riter - 1;
  std::cout << "rdiff is: " << rdiff << std::endl;

  auto iter = std::find(begin(vec), end(vec), 2);
  auto diff = iter - begin(vec);
  std::cout << "diff is: " << diff << std::endl;
  return 0;
}
