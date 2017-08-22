#ifndef ROSS_BLOCK_H_
#define ROSS_BLOCK_H_

#include "globals.h"
#include "ross_setup.h"

class BlockMapper : public LPMapper {
  uint64_t get_chare_id(uint64_t global_id) const {;
    return global_id / g_lps_per_chare;
  }
  uint64_t get_local_id(uint64_t global_id) const {
    return global_id % g_lps_per_chare;
  }
  uint64_t get_global_id(uint64_t chare_id, uint64_t local_id) const {
    return g_lps_per_chare * chare_id + local_id;
  }
  uint64_t get_num_lps(uint64_t chare_id) const {
    return g_lps_per_chare;
  }
};

#endif
