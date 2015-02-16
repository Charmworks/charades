#ifndef ROSS_SETUP_H_
#define ROSS_SETUP_H_

#include "typedefs.h"

#ifndef AVL_NODE_COUNT
#define AVL_NODE_COUNT 262144
#endif

#ifndef NUM_OUT_MESG
#define NUM_OUT_MESG 2000
#endif

void tw_init(int* argc, char*** argv);
void tw_define_lps(size_t msg_sz, tw_seed* seed);
void tw_run();
void tw_end();

#endif
