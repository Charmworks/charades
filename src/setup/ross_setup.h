#ifndef ROSS_SETUP_H_
#define ROSS_SETUP_H_

#include "typedefs.h"

void tw_init(int argc, char** argv);
void tw_define_lps(size_t msg_sz, tw_seed* seed);
void tw_run();
void tw_end();

#endif
