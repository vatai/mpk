typedef struct {
  int n;			/* number of nodes */
  int *ptr;			/* pointer */
  int *col;			/* column index */
} crs0_t;

crs0_t *new_crs(int, int);
void del_crs(crs0_t*);
crs0_t *read_crs(FILE*);
void write_crs(FILE*, crs0_t*);



typedef struct {
  crs0_t *g;
  double *x;
  double *y;
} coord_t;

coord_t *new_coord(crs0_t*);
void read_coord(FILE*, coord_t*);

typedef struct {
  crs0_t *g;			/* graph */
  int n_part;			/* number of parts */
  int *part;			/* partition */
} part_t;

part_t *new_part(crs0_t*);
void del_part(part_t*);
void read_part(FILE*, part_t*);
void write_part_c(FILE*, part_t*, coord_t*);

typedef struct {
  part_t *pg;
  int *level;
} level_t;

level_t *new_level(part_t*);
void del_level(level_t*);
void comp_level(level_t*);
void update_level(level_t *, level_t *);
void read_level(FILE *, level_t *);
void write_level(FILE*, level_t*);
void write_level_c(FILE*, level_t*, coord_t*);

typedef struct {
  crs0_t *g; // Associated graph
  int *wv;   // Weight on edges 
  int *we;   // Weight on edges 
} wcrs_t;

wcrs_t *new_wcrs(crs0_t*);
void del_wcrs(wcrs_t*);
void write_wcrs(FILE*, wcrs_t*);

void level2wcrs(level_t*, wcrs_t*);

typedef struct {
  part_t *pg;
  int *levels;
} skirt_t;

skirt_t *new_skirt(part_t*);
void del_skirt(skirt_t*);
void comp_skirt(skirt_t*);
void print_skirt_cost(skirt_t*, level_t*, int);
void read_skirt(FILE*, skirt_t*, int);
void write_skirt(FILE *f, skirt_t *sg, level_t *lg, int level);

typedef struct {
  int n;
  long *idx;
  int th;
  double t0, t1;
} task_t;

typedef struct {
  int n;
  int npart;
  int nlevel;
  int nphase;

  crs0_t *g0;
  part_t **plist;
  level_t **llist;
  skirt_t *sg;

  task_t *tlist;
  long *idxsrc;
  long idxallocsize;
} mpk_t;// used in new_mpk function in mpkread.c

typedef struct {
  int n;
  int nlevel;
  int npart;
  int nphase;
  // Each phase communicates a different amount, hence pointer to
  // pointer.  E.g. `vv_sbufs[phase][i]` is the `i`-th element of the
  // send buffer in phase `p`

  // Send and receive buffers for vertex values (from vv).
  double **vv_sbufs;
  double **vv_rbufs;
  // Send and receive buffers for (vv) indices.
  int **idx_sbufs;
  int **idx_rbufs;

  // `sendcounts[phase * npart + p]` and `recvcounts[phase * npart +
  // p]` are is the number of elements sent/received to/from partition
  // `p`.
  int *sendcounts;
  int *recvcounts;

  // `sdispls[phase * npart + p]` and `rsdispls[phase * npart + p]` is
  // the displacement (index) in the send/receive buffers where the
  // elements sent to partition/process `p` start.
  int *sdispls;
  int *rdispls;
} comm_data_t;

mpk_t *new_mpk(crs0_t*, int, int, int);
mpk_t *read_mpk(char*);
// TODO(vatai): void del_mpk(mpk_t*)// do it in mpkread.c
void prep_mpk(mpk_t*, double*);
void exec_mpk(mpk_t*, double*, int nthread);
void exec_mpk_xs(mpk_t*, double*, int nthread);
void exec_mpk_xd(mpk_t*, double*, int nthread);
void exec_mpk_is(mpk_t*, double*, int nthread);
void exec_mpk_id(mpk_t*, double*, int nthread);
void spmv_exec_seq(crs0_t*, double*, int nlevel);
void spmv_exec_par(crs0_t*, double*, int nlevel, int nth);
void mpi_exec_mpk(mpk_t *mg, double *vv, comm_data_t *cd, char *dir);
void mpi_prep_mpk(mpk_t*, double*, comm_data_t *);

void mpi_del_cd(comm_data_t *);
void mpi_prepbufs_mpk(mpk_t*, int comm_table[], comm_data_t*, int rank, int phase);
void print_values_of_vv(int rank, int phase, int n, int nlevel, double *vv, char *dir);
