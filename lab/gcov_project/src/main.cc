#include <iostream>

int factorial(const int n) { return n == 0 ? 1 : n * factorial(n - 1); }

int main(int argc, char *argv[]) {
  std::cout << "Hello, world!" << std::endl;
  std::cout << "Factorial: " << factorial(10) << std::endl;
  return 0;
}
