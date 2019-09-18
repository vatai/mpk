#ifndef _BUFFERS_T_H_
#define _BUFFERS_T_H_

#include <forward_list>

#include <metis.h>

class buffers_t {
public:
  std::forward_list<idx_t> idx_buf;
};

#endif
