#include <iostream>
#include <sstream>
#include <cstdio>
#include <cmath>
#include <algorithm>
// #include <stdlib.h>
// #include <string.h>
// #include "lib.h"
// #include "util.h"
#include "metis.h"

/**
 * Example ./dmpk data 4 4 10
 */

#include <string>
#include <fstream>
#include <cstddef>
#include <map>


#include <iomanip>

#include "leveledgraph.hpp"

int main(int argc, char **argv) {
  
  // process argv
  if (argc != 5 ) {
    std::cerr << "usage: " << argv[0] << " graph num_part num_iter num_steps" << std::endl;
    return 1;
  }

  idx_t num_part, num_iter, num_steps;
  std::stringstream ss;

  ss << argv[2]; ss >> num_part; ss.clear();
  ss << argv[3]; ss >> num_iter; ss.clear();
  ss << argv[4]; ss >> num_steps; ss.clear();

  LeveledGraph g(argv[1], num_part);
  bVector b(g.n, num_steps);

  for (int i = g.ptr[877]; i < g.ptr[878]; ++i) {
    std::cout << g.col[i]
              << ":"
              << std::setprecision(100)
              << g.val[i]
              << "*"
              << b.array[g.col[i]]
              << std::endl;
  }
  fp_t tmpsum=0;
  for (int i = g.ptr[877]; i < g.ptr[878]; ++i) tmpsum += b.array[g.col[i]] * g.val[i];
  std::cout << tmpsum << std::endl;

  g.partition();
  // g.printLevels();
  // g.printPartitions();

  // g.permute(b);
  // g.printPartitions();

  g.printHeader(num_iter,num_steps);
  for (int t = 0; t < num_iter; t++) {

    // std::cout << "MPK iteration: " << t << std::endl;
    g.MPK(b);

    g.printStats(t);
    // g.printLevels();
    if(t<2)
    g.printPartitions();

    g.updateWeights();
    g.wpartition();
    // g.printPartitions();
    g.optimisePartitions();
    
    // g.permute(b);
    // g.printPartitions();
  }
  //g.printLevels();
  std::cout << "% min level: " << *std::min_element(g.levels, g.levels + g.n-1) << std::endl;

  // g.inversePermute(b);

  b.octave_check("check.m"); 

  return 0;
}
