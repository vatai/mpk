#include "leveledgraph.h"

#include <iostream>
#include <map>
#include <cmath>
#include <cassert>

#include "metis.h"

bVector::bVector(int nn, int num_steps)
{
  int n = sqrt(nn);
  assert(n*n == nn);
  assert (n > 3);

  array = new fp_t[nn * num_steps ];

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


CSRMatrix::CSRMatrix(const char* s)
{
  // throw std::invalid_argument("Wrong file name");
  std::ifstream f(s) ;
  if(not f.is_open()) {
    std::cerr << "Invalid argument: could not open file." << std::endl;
    std::exit(1);
  }

  int i,j;
  fp_t v;
  std::map<std::pair<int, int>,fp_t> M;

  while (not (f >> n >> n >> nnz));
  while(not f.eof())
    if (f >> i >> j >> v)
      M[{i-1,j-1}] = v;

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
}

PartitionedGraph::~PartitionedGraph()
{
  delete [] partitions; partitions = nullptr;
}


LeveledGraph::LeveledGraph(const char* fn, const int np) :
  PartitionedGraph(fn, np)
{
  levels = new int[n];
  vertexweights = new int[n];
  edgeweights = new int[nnz];
  partials = new unsigned[n];
  currentPermutation = new unsigned[n];
  globalPermutation = new unsigned[n];
  inversePermutation = new unsigned[n];
  iterArray = new unsigned[n];
  partitionBegin = new unsigned[num_part+1];

  tmp_level = new int[n];
  tmp_part = new int[n];
  tmp_partial = new unsigned[n];
  tmp_perm = new unsigned[n];

  tmp_ptr = new int[n+1];
  tmp_col = new int[nnz];
  tmp_val = new fp_t[nnz];

  for (unsigned i = 0; i < n; ++i) {
    currentPermutation[i] = globalPermutation[i] = i;
    partials[i] = levels[i] = inversePermutation[i] = 0;
  }
  partition();
}
  
LeveledGraph::~LeveledGraph()
{
  delete [] levels; levels = nullptr;
  delete [] edgeweights; edgeweights = nullptr;
  delete [] vertexweights; vertexweights = nullptr;
  delete [] partials; partials = nullptr;
  delete [] currentPermutation; currentPermutation = nullptr;
  delete [] globalPermutation; globalPermutation = nullptr;
  delete [] inversePermutation; inversePermutation = nullptr;
  delete [] iterArray; iterArray = nullptr;

  delete [] tmp_level; tmp_level = nullptr;
  delete [] tmp_part; tmp_part = nullptr;
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
}

void LeveledGraph::wpartition()
{
  idx_t num_const = 1;
  idx_t dummy = 0;

  idx_t opt[METIS_NOPTIONS];
  METIS_SetDefaultOptions(opt);
  opt[METIS_OPTION_UFACTOR]=1000;

  METIS_PartGraphKway(&n, &num_const, ptr, col,
		      NULL, NULL, edgeweights, &num_part,
		      NULL, NULL, opt, &dummy, partitions);
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

void LeveledGraph::updatePermutation()
{
  unsigned i;
  for (i = 0; i < n; ++i) iterArray[i] = 0;
  for (i = 0; i < num_part+1; i++) partitionBegin[i] = 0;
  for (i = 0; i < n; i++)  partitionBegin[partitions[i] + 1]++;
  for (i = 0; i < num_part; i++) partitionBegin[i+1] += partitionBegin[i]; 

  for (i = 0; i < n; i++) {
    int p = partitions[i];
    int j = partitionBegin[p] + iterArray[p]; // new index
    currentPermutation[j] = i;
    inversePermutation[i] = j;
    iterArray[p]++; // adjust for next iteration
  }
}

void LeveledGraph::permute(fp_t** bb, unsigned num_steps)
{
  // checklist to permute
  // - perm[], part[], levels[], partials[]
  // - matrix col[] and ptr[]
  // - bb[] vector (k_step times)

  updatePermutation();
  int i = 0, k = 0;

  // memory for bb[] permutation.
  fp_t *tmp_bb = new fp_t[n * num_steps];
  

  // This is the main permutation loop: 
  for (i = 0; i < n; i++) {
 
    unsigned j = currentPermutation[i];

    // perm[], part[], level[] and partial[] array swaps.
    tmp_perm[i] = globalPermutation[j];
    tmp_part[i] = partitions[j];
    tmp_level[i] = levels[j];
    tmp_partial[i] = partials[j];
    
    // matrix ptr and col arrays
    tmp_ptr[i+1] = ptr[j+1] - ptr[j];
    for (idx_t t = ptr[j]; t < ptr[j+1]; t++) {
      int oldj = col[t];
      fp_t oldv = val[t];
      int newj = inversePermutation[oldj];
      tmp_val[k] = oldv;
      tmp_col[k] = newj;
      k++;
    }
    
    // bb[] array permutation (for each level)
    fp_t *bptr = *bb;
    fp_t *new_bptr = tmp_bb;
    for (idx_t t = 0; t < num_steps; t++) {
      new_bptr[i] = bptr[j];
      new_bptr += n;
      bptr += n;
    }
 
  }
  
  // prefix sum of ptr[] and col[] renaming
  tmp_ptr[0] = 0;
  for (i = 0; i < n; i++)
    tmp_ptr[i+1] += tmp_ptr[i];

  // perm[], part[], level[] and partial[] array cleanup.
  std::swap(globalPermutation, tmp_perm);
  std::swap(partitions, tmp_part);
  std::swap(levels, tmp_level);
  std::swap(partials, tmp_partial);
  

  // Matrix ptr[] and col[] array cleanup.
  std::swap(ptr, tmp_ptr);
  std::swap(col, tmp_col);
  std::swap(val, tmp_val);
  
  std::swap(*bb,tmp_bb);
  delete [] tmp_bb;
}

void LeveledGraph::inversePermute(fp_t** bb, unsigned num_steps)
{
  int i = 0, k = 0;

  // memory for bb[] permutation.
  fp_t *tmp_bb = new fp_t[n * num_steps];
  // This is the main permutation loop: 
  
  for (i = 0; i < n; i++) {
    
    unsigned j = globalPermutation[i];
    // perm[], part[], level[] and partial[] array swaps.
    tmp_perm[j] = globalPermutation[i];
    tmp_part[j] = partitions[i];
    tmp_level[j] = levels[i];
    tmp_partial[j] = partials[i];

    //* TODO
    // matrix ptr and col arrays
    // probably wrong! check!!!
    tmp_ptr[j+1] = ptr[i+1] - ptr[i];
    for (idx_t t = ptr[i]; t < ptr[i+1]; t++)
      tmp_col[k++] = globalPermutation[col[t]]; // probably wrong!!!
    //*/

    // bb[] array permutation (for each level)
    fp_t *bptr = *bb;
    fp_t *new_bptr = tmp_bb;
    for (idx_t t = 0; t < num_steps; t++) {
      new_bptr[j] = bptr[i];
      new_bptr += n;
      bptr += n;
    }
  }
  
  // prefix sum of tmp_ptr[]
  tmp_ptr[0] = 0;
  for (i = 0; i < n; i++)
    tmp_ptr[i+1] += tmp_ptr[i] ;

  // perm[], part[], level[] and partial[] array cleanup.
  std::swap(globalPermutation, tmp_perm);
  std::swap(partitions, tmp_part);
  std::swap(levels, tmp_level);
  std::swap(partials, tmp_partial);

  // Matrix ptr[] and col[] array cleanup.
  std::swap(ptr, tmp_ptr);
  std::swap(col, tmp_col);
  std::swap(val, tmp_val);

  std::swap(*bb, tmp_bb);
  delete [] tmp_bb;
  
}
//*/

void LeveledGraph::MPK(int level, bVector &bv)
{

  int i,j,k;
  // int sqrn = sqrt(n);

  fp_t* b = bv.array;
  fp_t* bb = b + n;;
  
  for (i = 0; i < n; i++) tmp_level[i] = levels[i];
  int lvl = 0;
  int contp = 1;
  
  for(lvl = 0; contp && lvl < level-1; lvl++) { // for each level
    contp = 0;
    for (i = 0; i < n; i++) { // for each node
      if (lvl == levels[i]){
	int i_is_complete = 1;
	// if (partials[i] == 0) bb[i] = b[i] * lg->pg->g->diag; // REMOVED IN CPP
	idx_t end_part = partitionBegin[partitions[i]+1];
	idx_t end = ptr[i+1];
	for (j = ptr[i]; j < end; j++) { // for each adj node
	  int diff = end - j;
	  assert(0 <= diff && diff<8);
	  k = col[j];
	  int same_part = (0 <=  end_part - k);
	  // if needed == not done
	  if (((1 << diff) & partials[i]) == 0) {
	    //if (same_part && lvl <= tmp_level[k] ){ 
            if (partitions[i] == partitions[k] && lvl <= tmp_level[k] ){ 

              bb[i] += b[k] * val[j];
	      partials[i] |= (1 << diff);
	    } else i_is_complete = 0;
	  } 
	} // end for each ajd node
	if(i_is_complete){
	  tmp_level[i]++; // level up
	  partials[i]=0; // no partials in new level
	  contp = 1;
	}
      } else contp = 1;
    }
    for (i = 0; i < n; i++) levels[i] = tmp_level[i];
    b = bb;
    bb += n;
  }
}

