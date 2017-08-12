#include "leveledgraph.hpp"

#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <climits>
#include "metis.h"

bVector::bVector(int nn, int ns) :
  n(nn),
  num_steps(ns)
{
  int n = sqrt(nn);
  assert(n*n == nn);
  assert (n > 3);

  array = new fp_t[nn * num_steps];

  int i=0;
  for (i = 0; i < num_steps*nn; ++i) array[i] = 0;

  i=0;
  while (i < n/3) {
    array[i] = 1;
    array[n*i] += 1;
    i++;
  }
  while (i < 2*(n/3)) {
    fp_t t= array[i-1] + 1;
    array[i] = t;
    array[n*i] += t;
    i++;
  }
  fp_t end = array[2*(n/3)-1]+1;
  while (i < n) {
    array[i] = end;
    array[n*i] += end;
    i++;
  }
  for (i = 0; i < n; i++) {
    array[n*i+n-1] += end;
    array[(n-1)*n + i] += end;
  }
}

bVector::~bVector()
{
  delete [] array; array = nullptr;
}

void bVector::octave_check(const char *fn)
{
  int sqrn = sqrt(n);
  std::ofstream f(fn);
  f << "1;" << std::endl
    << "n = " << sqrn << ";pde;" << std::endl
    << "rv = {" << std::endl;

  for (int t = 0; t < num_steps; t++) {
    f //<< "rv" << t
      << " [";
    for (int i = 0; i < n; i++) {
      if (i % sqrn == 0)     f << std::endl;
      f <<  " " << std::fixed << std::setw(5) << std::setprecision(1) << array[t*n+i];
    }
    f << "];\n";
  }
  f << "};" << std::endl;
  
  f << "tmp = bb;" << std::endl
    << "for i=1:" << num_steps << std::endl
    << "  norm(rv{i}-reshape(tmp," << sqrn << "," << sqrn << "))" << std::endl
    << "  tmp=A*tmp;" << std::endl
    << "endfor" << std::endl;
  f.close();
}


CSRMatrix::CSRMatrix(const char* s)
{
  // throw std::invalid_argument("Wrong file name");
  std::ifstream f(s) ;
  if(not f.is_open()) {
    std::cerr << "Invalid argument: could not open file." << std::endl;
    std::exit(1);
  }


  std::string header, matrix, storage, type, pattern;
  f >> header >> matrix >> storage >> type >> pattern;
  if (header != "%%MatrixMarket") {
    std::cerr << "Invalid file format: " << header << std::endl;
    std::exit(1);
  }
  bool sym = pattern == "symmetric";

  while (not (f >> n >> n >> nnz));

  int i,j;
  fp_t v;
  std::map<std::pair<int, int>,fp_t> M;
  while(not f.eof())
    if (f >> i >> j >> v) {
      M[{i-1,j-1}] = v;
      if (sym) M[{j-1,i-1}] = v;
    }
  ptr = new int[n+1];
  col = new int[nnz];
  val = new fp_t[nnz];

  int cnt = 0;
  ptr[cnt] = 0;
  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j) {
      if (M.count({i,j})) {
        col[cnt]=j;
        val[cnt]=M[{i,j}];
        cnt++;
      }
      ptr[i+1] = cnt;
    }
  }

  f.close();
}

CSRMatrix::~CSRMatrix()
{
  delete [] ptr; ptr = nullptr;
  delete [] col; col = nullptr;
  delete [] val; val = nullptr;
}

PartitionedGraph::PartitionedGraph(const char* fn, const int npart) :
  CSRMatrix(fn), num_part(npart)
{
  partitions = new int[n];
  old_partitions = new int[n];
  for (int i = 0; i < n; ++i) partitions[i] = old_partitions[i] = 0;

}

PartitionedGraph::~PartitionedGraph()
{
  delete [] partitions; partitions = nullptr;
  delete [] old_partitions; old_partitions = nullptr;
}


LeveledGraph::LeveledGraph(const char* fn, const int np) :
  PartitionedGraph(fn, np)
{
  levels = new int[n];
  old_levels = new int[n];
  vertexweights = new int[n];
  edgeweights = new int[nnz];

  partials = new unsigned[n];
  currentPermutation = new unsigned[n];
  globalPermutation = new unsigned[n];
  inversePermutation = new unsigned[n];
  iterArray = new unsigned[n];
  partitionBegin = new unsigned[num_part+1];

  tmp_level = new int[n];
  tmp_old_level = new int[n];
  tmp_part = new int[n];
  tmp_old_part = new int[n];
  tmp_partial = new unsigned[n];
  tmp_perm = new unsigned[n];

  tmp_ptr = new int[n+1];
  tmp_col = new int[nnz];
  tmp_val = new fp_t[nnz];

  for (unsigned i = 0; i < n; ++i) {
    currentPermutation[i] = globalPermutation[i] = inversePermutation[i] = i;
    partials[i] = tmp_partial[i] = levels[i] = old_levels[i] = 0;
  }
  partition();
}
  
LeveledGraph::~LeveledGraph()
{
  delete [] levels; levels = nullptr;
  delete [] old_levels; levels = nullptr;
  delete [] edgeweights; edgeweights = nullptr;
  delete [] vertexweights; vertexweights = nullptr;
  delete [] partials; partials = nullptr;
  delete [] currentPermutation; currentPermutation = nullptr;
  delete [] globalPermutation; globalPermutation = nullptr;
  delete [] inversePermutation; inversePermutation = nullptr;
  delete [] iterArray; iterArray = nullptr;

  delete [] tmp_level; tmp_level = nullptr;
  delete [] tmp_old_level; tmp_level = nullptr;
  delete [] tmp_part; tmp_part = nullptr;
  delete [] tmp_old_part; tmp_old_part = nullptr;
  delete [] tmp_partial; tmp_partial = nullptr;
  delete [] tmp_perm; tmp_perm = nullptr;

  delete [] tmp_ptr; tmp_ptr = nullptr;
  delete [] tmp_col; tmp_col = nullptr;
  delete [] tmp_val; tmp_val = nullptr;
}

void LeveledGraph::partition()
{
  idx_t num_const = 1;
  idx_t dummy = 0;
  METIS_PartGraphKway(&n, &num_const, ptr, col,
                      NULL, NULL, NULL, &num_part,
                      NULL, NULL, NULL, &dummy, partitions);
  std::copy(partitions, partitions+n, old_partitions);
}

void LeveledGraph::printPartitions()
{
  std::cout << "Partitions:" << std::endl;
  printArray(partitions);
};

void LeveledGraph::printLevels()
{
  std::cout << "Levels:" << std::endl;
  printArray(levels);
}

template <typename T>
void LeveledGraph::printArray(T *array)
{
  int sn = sqrt(n);
  for (int i = 0; i < n; ++i) {
    if (i !=0 and not (i%sn)) std::cout << std::endl; 
    std::cout << std::setw(4) << array[i];
  }
  std::cout << std::endl;
}


void LeveledGraph::wpartition()
{
  idx_t num_const = 1;
  idx_t dummy = 0;

  idx_t opt[METIS_NOPTIONS];
  METIS_SetDefaultOptions(opt);
  opt[METIS_OPTION_UFACTOR]=1000;
  
  std::swap(old_partitions,partitions);
  METIS_PartGraphKway(&n, &num_const, ptr, col,
		      NULL, NULL, edgeweights, &num_part,
		      NULL, NULL, opt, &dummy, partitions);

}

void LeveledGraph::optimisePartitions() {
  // Optimise the new partitioning!!! ////////
  int i,j;
  
  // Who goes where!
  // count[i,j] says how many vertices of partition i went into the
  // new partiton j.
  std::map<std::pair<int,int>,int> count;

  for (i = 0; i < n; ++i)
    ++count[{old_partitions[i],partitions[i]}];

#define NDEBUG
#ifndef NDEBUG
  std::cout << "Partition optimisation:" << std::endl;
  for (i = 0; i < num_part; ++i) {
    for (j= 0; j < num_part; ++j)
      std::cout << std::setw(3) << count[{i,j}];
    std::cout << std::endl;
  }
#endif

  //#define TWO
#ifdef TWO
  // by knuth
  std::vector<int> rename(num_part), a(num_part);
  int max = 0;
  for (i = 0; i < num_part; ++i) {
    a[i] = rename[i] = i;
    max += count[{i,rename[i]}];
  }

  while (true) {
    // L1 visit 0,..,num_part
    int tmax = 0;
    for (i = 0; i < num_part; ++i) 
      tmax += count[{i,a[i]}];
    if (tmax>max) {
      max = tmax;
      std::copy(a.begin(), a.end(), rename.begin());
    }

    // L2 Find j = i+1
    i = num_part-2;
    while (a[i] >= a[i+1] and i >= 0) i--;
    // now a[i] < a[i+1]
    if (i == -1) break;

    // L3 Increase a i
    j = num_part-1;
    while (a[i] >= a[j]) j--;
    std::swap(a[i],a[j]);

    // reverse
    int k = i+1; j = num_part-1;
    while (k < j) {
      std::swap(a[k],a[j]);
      k++;j--;
    }
  }
#endif

  
#ifndef TWO
  // Figure out the new order!
  //std::vector<std::map<int,std::vector<int>>> suggestions(num_part);
  std::map<int,std::vector<std::pair<int,int>>,std::greater<int>> suggestions;
  for (i = 0; i < num_part; ++i) {
    for (j = 0; j < num_part; ++j) {
      int x = count[{i,j}];
      suggestions[x].push_back({i,j});
    }
  }
    
  std::set<int> donei,donej;
  std::map<int,int> rename;
  int cand = 0;
  while ( donei.size() < num_part or donej.size() < num_part) {
    auto it = suggestions.begin();
    while ( it != suggestions.end()) {
      if (cand < it->second.size()) {
        i = it->second[cand].first;
        j = it->second[cand].second;
        if (donei.find(i) == donei.end() and
            donej.find(j) == donej.end()) {
          // not in done
          donei.insert(i);
          donej.insert(j);
          rename[i]=j;
        }
      }
      it++;
    }
    cand++; // next candidate in list
  }
#endif
  
  //for (i = 0; i < num_part; ++i) std::cout << rename[i]
  for (i = 0; i < n; ++i) {
    int p = partitions[i];
    partitions[i] = rename[p];
  }
#ifndef NDEBUG
  for (i = 0; i < num_part; ++i) std::cout << rename[i] << ", ";
  std::cout << std::endl;
#endif
}

void LeveledGraph::updateWeights()
{
  int max,min;
  int i, j;
    
  max = min = levels[0];
  for (i = 1; i < n; ++i) {
    if (max < levels[i]) max = levels[i];
    if (min > levels[i]) min = levels[i];
  }

  for (i = 0; i < n; ++i) {
    vertexweights[i] = max + 1 - levels[i];

    int l0 = levels[i];
    for (j = ptr[i]; j < ptr[i+1]; ++j) {
      int l1 = levels[col[j]];
      double w = l0 + l1 - 2 * min;
      w = 1e+6 / (w + 1);
      if (w < 1.0) edgeweights[j] = 1;
      else edgeweights[j] = w;
    }
  }
}

void LeveledGraph::permute(bVector& bvect)
{

  int i;
  // Initialise iterArray.
  for (i = 0; i < n; ++i) iterArray[i] = 0;
  // Calculate partitionBegin[].
  for (i = 0; i < num_part+1; i++) partitionBegin[i] = 0;
  for (i = 0; i < n; i++)  partitionBegin[partitions[i] + 1]++;
  for (i = 0; i < num_part; i++) partitionBegin[i+1] += partitionBegin[i]; 
  // Calculate currentPermutation[] and inversePermutation[].
  // iterArray[] is auxiliary array, iterArray[p] counts how many of
  // indices with partition p are in their place, so partitionBegin[p]
  // + iterArray[p] is the new index of the vertex i with
  // partition[p].
  for (i = 0; i < n; i++) {
    int p = partitions[i];
    int j = partitionBegin[p] + iterArray[p]; // new index
    currentPermutation[j] = i;
    inversePermutation[i] = j;
    iterArray[p]++; // adjust for next iteration
  }

  //// Apply currentPermutation[] and/or inversePermutation[] to all
  //// data-structures.
  
  // memory for bb[] permutation.
  fp_t *tmp_bb = new fp_t[bvect.n * bvect.num_steps];

  // This is the main permutation loop: 
  int k = 0;
  for (i = 0; i < n; i++) {
 
    unsigned j = currentPermutation[i];

    tmp_perm[i] = globalPermutation[j];
    tmp_part[i] = partitions[j];
    tmp_old_part[i] = old_partitions[j];
    tmp_level[i] = levels[j];
    tmp_old_level[i] = old_levels[j];
    tmp_partial[i] = partials[j];
    
    // Permute matrix ptr, col and val arrays.  Note: ptr is only
    // partially done.
    tmp_ptr[i+1] = ptr[j+1] - ptr[j];
    for (idx_t t = ptr[j]; t < ptr[j+1]; t++) {
      int oldj = col[t];
      fp_t oldv = val[t];
      // Don't know which one is faster... probably inversePermutation[].
      int newj = inversePermutation[oldj];
      // int newj = std::distance(currentPermutation, std::find(currentPermutation, currentPermutation+n, oldj));
      tmp_val[k] = oldv;
      tmp_col[k] = newj;
      k++;
    }
    
    // bb[] array permutation (for each level)
    fp_t *old_bptr = bvect.array;
    fp_t *new_bptr = tmp_bb;
    for (idx_t t = 0; t < bvect.num_steps; t++) {
      new_bptr[i] = old_bptr[j];
      new_bptr += n;
      old_bptr += n;
    }
  }
  
  // Finish ptr by converting taking the prefix sum.
  tmp_ptr[0] = 0;
  for (i = 0; i < n; i++)
    tmp_ptr[i+1] += tmp_ptr[i];

  // Put the permuted array in the place of the original ones.
  std::swap(globalPermutation, tmp_perm);
  std::swap(partitions, tmp_part);
  std::swap(old_partitions, tmp_old_part);
  std::swap(levels, tmp_level);
  std::swap(old_levels, tmp_old_level);
  std::swap(partials, tmp_partial);
  
  std::swap(ptr, tmp_ptr);
  std::swap(col, tmp_col);
  std::swap(val, tmp_val);
  
  std::swap(bvect.array,tmp_bb);
  delete [] tmp_bb;
}

void LeveledGraph::inversePermute(bVector &bvect)
{

  // memory for bb[] permutation.
  fp_t *tmp_bb = new fp_t[bvect.n * bvect.num_steps];

  // This is the main (inverse) permutation loop:
  int i = 0, k = 0;
  for (i = 0; i < n; i++) {
    
    unsigned j = globalPermutation[i];

    tmp_perm[j] = globalPermutation[i];
    tmp_part[j] = partitions[i];
    tmp_old_part[j] = old_partitions[i];
    tmp_level[j] = levels[i];
    tmp_old_level[j] = old_levels[i];
    tmp_partial[j] = partials[i];

    // This this should inverte the permutation on the matrix, but it
    // might be wrong... and actually it is not needed.
    tmp_ptr[j+1] = ptr[i+1] - ptr[i];
    for (idx_t t = ptr[i]; t < ptr[i+1]; t++) {
      int oldj = col[t];
      fp_t oldv = val[t];
      unsigned* invptr = std::find(globalPermutation, globalPermutation+n, oldj);
      int newj = std::distance(globalPermutation, invptr);
      tmp_val[k] = oldv;
      tmp_col[k] = newj;
      k++;
    }

    // bb[] array permutation (for each level)
    fp_t *bptr = bvect.array;
    fp_t *new_bptr = tmp_bb;
    for (idx_t t = 0; t < bvect.num_steps; t++) {
      new_bptr[j] = bptr[i];
      new_bptr += n;
      bptr += n;
    }
  }
  
  // prefix sum of tmp_ptr[]
  tmp_ptr[0] = 0;
  for (i = 0; i < n; i++)
    tmp_ptr[i+1] += tmp_ptr[i] ;

  // Put the permuted array in the place of the original ones.
  std::swap(globalPermutation, tmp_perm);
  std::swap(partitions, tmp_part);
  std::swap(old_partitions, tmp_old_part);
  std::swap(levels, tmp_level);
  std::swap(old_levels, tmp_old_level);
  std::swap(partials, tmp_partial);

  std::swap(ptr, tmp_ptr);
  std::swap(col, tmp_col);
  std::swap(val, tmp_val);

  std::swap(bvect.array, tmp_bb);
  delete [] tmp_bb;
}

void LeveledGraph::MPK(bVector &bv)
{
  comm_log.clear();
  int i = 0, j = 0, k = 0;
  // int sqrn = sqrt(n);

  fp_t* b = bv.array;
  fp_t* bb = b + n;;
  int lvl = 0;
  int contp = 1;

  int *not_sent = new int[bv.n * bv.num_steps];
  for (i = 0; i < bv.n * bv.num_steps; ++i) not_sent[i] = true;
  
  for(lvl = 0; contp && lvl < bv.num_steps-1; lvl++) { // for each level
    contp = 0;
    for (i = 0; i < n; i++) { // for each node
      if (lvl == levels[i]) {
        //*
        int op = old_partitions[i], np = partitions[i];
        if (op != np and old_levels[i] == lvl and
            partials[i] != 0 and not_sent[(lvl+1)*bv.n+i]) {
          not_sent[(lvl+1)*bv.n+i]=false;
          ++comm_log[{op, np}];
          // if(op==1&&np==2)std::cout<<"v="<<i<<":"<<op<<"->"<<np<<"(lvl="<<lvl<<")"<<std::endl;
        }

	int i_is_complete = 1;
	// idx_t end_part = partitionBegin[partitions[i]+1];
	idx_t end = ptr[i+1];

	for (j = ptr[i]; j < end; j++) { // for each adj node
	  int diff = end - j;
	  assert(0 <= diff and (diff<sizeof(*partials)*CHAR_BIT));
	  k = col[j];

	  // int same_part = (0 <=  end_part - k);
	  // if needed == not done
	  if (((1 << diff) & partials[i]) == 0) {
	    //if (same_part and lvl <= levels[k] ){ 
            if (partitions[i] == partitions[k] and lvl <= levels[k] ) { 
              bb[i] += b[k] * val[j];
	      partials[i] |= (1 << diff);
              op = old_partitions[k];
              np = partitions[i];
              if (op != np
                  and old_levels[k] >= lvl
                  and not_sent[lvl*bv.n+k]) {
                not_sent[lvl*bv.n+k] = false;
                ++comm_log[{op, np}];
                // if(op==1&&np==2)std::cout<<"v="<<k<<"(>"<<i<<"):"<<op<<">"<<np<<"("<<lvl<<")"<<std::endl;
              }
	    } else i_is_complete = 0;
	  } 
	} // end for each ajd node

	if(i_is_complete){
          // std::cout << i << " level up" << std::endl;
	  levels[i]++; // level up
	  partials[i]=0; // no partials in new level
	  contp = 1;
	}
      } else contp = 1;

    }
    //for (i = 0; i < n; i++) levels[i] = tmp_level[i];
    b = bb;
    bb += n;
  }

  for (i = 0; i < n; i++) old_levels[i] = levels[i];
  delete [] not_sent;
}

void LeveledGraph::printStats()
{
  int i,j;
  /*
  for (i = 0; i < num_part; ++i) {
    for (j = 0; j < num_part; ++j) 
      std::cout << std::setw(3) << comm_log[{i,j}] << ",";
    std::cout << std::endl;
  }
  //*/
  int count=0;
  int sum = 0, max, min;
  min = max = comm_log[{0,0}];

  typedef std::pair<std::pair<int,int>,int> stats_t;
  
  auto begin = comm_log.begin(), end = comm_log.end();
  count = std::count_if(begin, end, [](stats_t t){return t.second ;});
  if (not count) return;
  sum = std::accumulate(begin,end,0,
                        [](const int previous, const stats_t& p)
                        { return previous+p.second; });
  max = std::max_element(begin,end,[](auto a, auto b) {return a.second < b.second;})->second;
  min = max;

  for (i = 0; i < num_part; ++i) 
    for (j = 0; j < num_part; ++j) {
      int cur = comm_log[{i,j}];
      if (min>cur and cur) min=cur;
    }

  std::cout << std::setw(5) << "&" << std::setw(5) << min
            << std::setw(5) << "&" << std::setw(5) << max
            << std::setw(5) << "&" << std::setw(5) << count
            << std::setw(5) << "&" << std::setw(5) << sum
            << std::setw(5) << "&" << std::setw(8)
            << std::setprecision(1) << std::fixed << float(sum)/float(count)
            << std::setw(5) << "\\\\" << std::endl;
}
