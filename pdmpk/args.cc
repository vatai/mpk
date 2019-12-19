#include "args.h"
#include <stdexcept>
#include <string>

Args::Args(int &argc, char *argv[]) {
  // Number of arguments.
  if (argc != 4)
    throw std::logic_error("Wrong number of arguments");

  // File name.
  mtxname = argv[1];

  // Check extension.
  if (std::string(".mtx") != std::string(mtxname.end() - 4, mtxname.end()))
    throw std::logic_error("Input file doesn't end with mtx");

  // npart and nlevel.
  npart = std::stoi(argv[2]);
  nlevels = std::stoi(argv[3]);
}
