#include <iostream>
#include <sstream>
#include <cstdio>
#include <cmath>
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

void octave_check(const char *fn, fp_t* b, int N, int k){
  int n = sqrt(N);
  FILE* f = fopen(fn,"w");
  fprintf(f,"1;\nn = %d;pde;\n", n);
  //b += N;
  for (int t = 0; t < k; t++) {
    fprintf(f,"rv%d = reshape(A^%d*bb,%d,%d) - [", t, t, n,n);
    for (int i = 0; i < N; i++) {
      if (i % n == 0)     fprintf(f,"\n");
      fprintf(f, " %6.1f", b[i]);
    }
    fprintf(f,"]; \nnorm(rv%d)\n",t);
    b += N;
  }
  fclose(f);
}


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
  g.partition();

  bVector b(g.n, num_steps);

  int i;  
  
  // prn_lvl(lg, bb, 0);
  for (int t = 0; t < num_iter; t++) {
    // iwrite("part", argv[1], t, (void*)pg);
    g.permute(&b.array, num_steps);
    g.MPK(num_steps, b);
    // misc_info(lg, bb, k_steps, t);
    g.updateWeights();
    g.wpartition();
  }

  g.inversePermute(&b.array, num_steps);
  
  octave_check("check.m", b.array, g.n, num_steps); 

  std::cout << "bye4" << std::endl;
  return 0;
}
