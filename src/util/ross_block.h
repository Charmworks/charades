#ifndef ROSS_BLOCK_H_
#define ROSS_BLOCK_H_

#include "typedefs.h"

LPID init_block_map(unsigned chare, LPID local_id);
unsigned chare_block_map(LPID global_id);
LPID local_block_map(LPID global_id);
unsigned constant_numlp_map(unsigned chare);

#endif
