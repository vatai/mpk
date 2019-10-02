#include <iostream>
#include <fstream>
#include <string>

void read()
{
  char fname[] = "dbls.bin";
  std::ifstream file {fname, std::ios::binary};
  if (file.is_open()) {
    double val;
    file.read(reinterpret_cast<char*>(&val), sizeof(val));
    std::cout << "Read " << val << " from file " << fname << std::endl;
  }
}

void write()
{
  char fname[] = "dbls.bin";
  std::ofstream file {fname, std::ios::binary};
  std::cout << "Enter float/double: ";
  double val;
  std::cin >> val;
  file.write(reinterpret_cast<char*>(&val), sizeof(val));
}

int main(int argc, char *argv[])
{
  read();
  write();
  return 0;
}
