#ifndef _CHARM_FUNCTIONS_H_
#define _CHARM_FUNCTIONS_H_

void charm_init(int argc, char** argv);
void charm_run();
void charm_exit();

int tw_ismaster();
int tw_nnodes();

int tw_mype();
void tw_abort(const char*);

#ifdef DEBUG_ON
#define DEBUG(format, ...) { CkPrintf("DEBUG: "format, ## __VA_ARGS__); }
#else
#define DEBUG(format, ...) { }
#endif

#ifdef DEBUG_MASTER_ON
#define DEBUG_MASTER(format, ...) { if(tw_ismaster()) CkPrintf("MASTER: "format, ## __VA_ARGS__); }
#else
#define DEBUG_MASTER(format, ...) { }
#endif

#ifdef DEBUG_LP_ON
#define DEBUG_LP(format, ...) { CkPrintf("LP[%d]: "format, thisIndex, ## __VA_ARGS__); }
#else
#define DEBUG_LP(format, ...) {}
#endif

#endif
