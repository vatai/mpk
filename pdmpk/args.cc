#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <string>

#include "args.h"

Args::Args(int &argc, char *argv[]) : npart{0}, nlevels(0) {
  METIS_SetDefaultOptions(opt);
  opt[METIS_OPTION_UFACTOR] = 1e+9;
  opt[METIS_OPTION_CONTIG] = 0;

  if (const char *ompi_npart = std::getenv("OMPI_COMM_WORLD_SIZE")) {
    npart = std::stoi(ompi_npart);
  };

  struct option long_options[] = {
      {"matrix", required_argument, 0, 'm'},
      {"npart", required_argument, 0, 'p'},
      {"nlevel", required_argument, 0, 'l'},
      {0, 0, 0, 0} // last element must be all 0s
  };
  int option_index = 0;

  while (1) {
    char c = getopt_long(argc, argv, "m:p:l:", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'm':
      mtxname = optarg;
      if (std::string(".mtx") != std::string(mtxname.end() - 4, mtxname.end()))
        throw std::logic_error("Input file doesn't end with mtx");
      break;
    case 'p':
      if (npart) {
        throw std::logic_error("Using --npart (or -p) is "
                               "not allowed when using MPI.");
      }
      npart = std::stoi(optarg);
      break;
    case 'l':
      nlevels = std::stoi(optarg);
      break;

    case '?':
      exit(1);
    default:
      throw std::logic_error("Error with getopt_long()");
    }
  }
  if (mtxname == "") {
    throw std::logic_error("No marix provided (use --matrix)");
  }
  if (npart == 0) {
    throw std::logic_error("Number of partitions not provided (use --npart)");
  }
  if (nlevels == 0) {
    throw std::logic_error("Number of levels not provided (use --nlevel)");
  }
}
