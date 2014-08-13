#ifndef ROSS_SETUP_H_
#define ROSS_SETUP_H_

#include "../typedefs.h"

#define AVL_NODE_COUNT 262144

void tw_init(int* argc, char*** argv);
void tw_define_lps(tw_lpid nlp, size_t msg_sz, tw_seed* seed);

#endif
