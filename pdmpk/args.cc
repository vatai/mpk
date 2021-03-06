#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <metis.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>

#include "args.h"

#define IF_OPTION_VALUE(OPTION, VALUE)                                         \
  if (std::string("METIS_" #OPTION "_" #VALUE) == optarg) {                    \
    opt[METIS_OPTION_##OPTION] = METIS_##OPTION##_##VALUE;                     \
  } else

#define ELSE_THROW(OPTION, VALUES)                                             \
  {                                                                            \
    throw std::logic_error("Wrong --" #OPTION " (should be "                   \
                           "METIS_" #OPTION "_{" VALUES "})");                 \
  };                                                                           \
  break;

Args::Args(int argc, char *argv[])
    : npart{0}, nlevel{0}, mirror_method{1}, weight_update_method{0},
      keepfiles(false) {
  METIS_SetDefaultOptions(default_opt);
  METIS_SetDefaultOptions(opt);
  // opt[METIS_OPTION_UFACTOR] = 1000; // originally 1000
  // opt[METIS_OPTION_CONTIG] = 0;
  // opt[METIS_OPTION_MINCONN] = 1;
  GetEnvArgs();
  ReadArgs(argc, argv);
}

void Args::GetEnvArgs() {
  if (const char *ompi_npart = std::getenv("PMI_SIZE")) {
    npart = std::stoi(ompi_npart);
  };
  if (const char *ompi_npart = std::getenv("OMPI_COMM_WORLD_SIZE")) {
    npart = std::stoi(ompi_npart);
  };
}

void Args::ReadArgs(int argc, char *argv[]) {
  struct option long_options[] = {
      {"matrix", required_argument, 0, 'm'},
      {"npart", required_argument, 0, 'n'},
      {"nlevel", required_argument, 0, 'l'},
      {"mirror", required_argument, 0, 'M'},
      {"weight", required_argument, 0, 'w'},
      // Metis options.
      {"PTYPE", required_argument, 0, 't'},
      {"OBJTYPE", required_argument, 0, 'o'},
      {"CTYPE", required_argument, 0, 'y'},
      {"IPTYPE", required_argument, 0, 'i'},
      {"RTYPE", required_argument, 0, 'r'},
      // ---
      {"NCUTS", required_argument, 0, 's'},
      {"NSEPS", required_argument, 0, 'p'},
      {"NUMBERING", required_argument, 0, 'b'},
      {"NITER", required_argument, 0, 'e'},
      {"SEED", required_argument, 0, 'd'},
      // ---
      {"MINCONN", required_argument, 0, 'c'},
      {"NO2HOP", required_argument, 0, 'h'},
      {"CONTIG", required_argument, 0, 'g'},
      {"COMPRESS", required_argument, 0, 'C'},
      {"CCORDER", required_argument, 0, 'R'},
      // ---
      {"PFACTOR", required_argument, 0, 'P'},
      {"UFACTOR", required_argument, 0, 'u'},
      // others
      {"keepfiles", no_argument, 0, 'K'},
      {0, 0, 0, 0} // last element must be all 0s
  };
  int option_index = 0;

  while (1) {
    char c = getopt_long(argc, argv,
                         "m:n:l:M:w:"
                         "t:o:y:i:r:"
                         "s:p:b:e:d:"
                         "c:h:g:C:R:"
                         "P:u:"
                         "K",
                         long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
      // ---- "m:n:l:M:w:" ----
    case 'm': // --matrix
      mtxname = optarg;
      if (std::string(".mtx") != std::string(mtxname.end() - 4, mtxname.end()))
        throw std::logic_error("Input file doesn't end with mtx");
      break;
    case 'n': // --npart
      if (npart) {
        throw std::logic_error("Using --npart (or -p) is "
                               "not allowed when using MPI.");
      }
      npart = std::stoi(optarg);
      break;
    case 'l': // --nlevel
      nlevel = std::stoi(optarg);
      break;
    case 'M':
      mirror_method = std::stoi(optarg);
      break;
    case 'w':
      weight_update_method = std::stoi(optarg);
      break;
      // ---- "t:o:y:i:r:" ----
    case 't': // --PTYPE
      IF_OPTION_VALUE(PTYPE, RB)
      IF_OPTION_VALUE(PTYPE, KWAY)
      ELSE_THROW(PTYPE, "RB, KWAY")
    case 'o': // --OBJTYPE
      IF_OPTION_VALUE(OBJTYPE, CUT)
      IF_OPTION_VALUE(OBJTYPE, VOL)
      ELSE_THROW(OBJTYPE, "CUT, VOL")
    case 'y': // --CTYPE
      IF_OPTION_VALUE(CTYPE, RM)
      IF_OPTION_VALUE(CTYPE, SHEM)
      ELSE_THROW(CTYPE, "RM, SHEM")
    case 'i': // --IPTYPE
      IF_OPTION_VALUE(IPTYPE, GROW)
      IF_OPTION_VALUE(IPTYPE, RANDOM)
      IF_OPTION_VALUE(IPTYPE, EDGE)
      IF_OPTION_VALUE(IPTYPE, NODE)
      ELSE_THROW(IPTYPE, "GROW, RANDOM, EDGE, NODE")
    case 'r':
      IF_OPTION_VALUE(RTYPE, FM)
      IF_OPTION_VALUE(RTYPE, GREEDY)
      IF_OPTION_VALUE(RTYPE, SEP2SIDED)
      IF_OPTION_VALUE(RTYPE, SEP1SIDED)
      ELSE_THROW(RTYPE, "FM, GREEDY, SEP2SIDED, SEP1SIDED")
      // ---- "s:p:b:e:d:" ----
    case 's':
      opt[METIS_OPTION_NCUTS] = std::stoi(optarg);
      break;
    case 'p':
      opt[METIS_OPTION_NSEPS] = std::stoi(optarg);
      break;
    case 'b':
      opt[METIS_OPTION_NUMBERING] = std::stoi(optarg);
      break;
    case 'e':
      opt[METIS_OPTION_NITER] = std::stoi(optarg);
      break;
    case 'd':
      opt[METIS_OPTION_SEED] = std::stoi(optarg);
      break;
      // ---- "c:h:g:C:R:" ----
    case 'c':
      opt[METIS_OPTION_MINCONN] = std::stoi(optarg);
      break;
    case 'h':
      opt[METIS_OPTION_NO2HOP] = std::stoi(optarg);
      break;
    case 'g':
      opt[METIS_OPTION_CONTIG] = std::stoi(optarg);
      break;
    case 'C':
      opt[METIS_OPTION_COMPRESS] = std::stoi(optarg);
      break;
    case 'R':
      opt[METIS_OPTION_CCORDER] = std::stoi(optarg);
      break;
    // ---- "P:u:", ----
    case 'P':
      opt[METIS_OPTION_PFACTOR] = std::stoi(optarg);
      break;
    case 'u':
      opt[METIS_OPTION_UFACTOR] = std::stoi(optarg);
      break;
    // ---- other ----
    case 'K':
      keepfiles = true;
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
  if (nlevel == 0) {
    throw std::logic_error("Number of levels not provided (use --nlevel)");
  }
}

std::string Args::Filename(const std::string &suffix, const int &rank) const {
  std::stringstream ss;
  ss << mtxname << "-np" << npart << "-nl" << nlevel << "-mm" << mirror_method
     << "-wm" << weight_update_method;
  if (rank > -1) {
    ss << "-r" << std::to_string(rank);
  }
  for (size_t k = 0; k < METIS_NOPTIONS; k++) {
    if (opt[k] != default_opt[k]) {
      ss << "-k" << k << "v" << opt[k];
    }
  }
  ss << "-" << GIT_BRANCH << "-" << GIT_COMMIT_HASH;
  ss << "." << suffix;
  return ss.str();
}

json Args::ToJson() const {
  json j;
  std::string name = mtxname;
  name.erase(0, name.find_last_of('/') + 1);
  j["matrix"] = name;
  j["npart"] = npart;
  j["nlevel"] = nlevel;
  j["mirror_method"] = mirror_method;
  j["weight_update_method"] = weight_update_method;
  std::map<idx_t, idx_t> active_ops;
  for (size_t k = 0; k < METIS_NOPTIONS; k++) {
    if (default_opt[k] != opt[k]) {
      active_ops[k] = opt[k];
    }
  }
  j["metis_opts"] = active_ops;
  return j;
}
