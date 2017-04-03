#ifndef ROSS_BLOCK_H_
#define ROSS_BLOCK_H_

#include "typedefs.h"

tw_lpid init_block_map(unsigned chare, tw_lpid local_id);
unsigned chare_block_map(tw_lpid global_id);
tw_lpid local_block_map(tw_lpid global_id);
unsigned constant_numlp_map(unsigned chare);

#endif
