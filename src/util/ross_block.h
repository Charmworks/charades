#ifndef ROSS_BLOCK_H_
#define ROSS_BLOCK_H_

#include "typedefs.h"

uint64_t init_block_map(uint64_t chare, uint64_t local_id);
uint64_t chare_block_map(uint64_t global_id);
uint64_t local_block_map(uint64_t global_id);
uint64_t constant_numlp_map(uint64_t chare);

#endif
