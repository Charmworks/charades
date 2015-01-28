#ifndef _CHARM_FUNCTIONS_H_
#define _CHARM_FUNCTIONS_H_

void charm_init(int argc, char** argv);
void charm_run();
void charm_exit();

int tw_ismaster();
int tw_nnodes();

int tw_mype();
void tw_abort(const char*);

#define DEBUG(format, ...) { }
#define DEBUG_MASTER(format, ...) { }
//#define DEBUG(format, ...) { CkPrintf(format, ## __VA_ARGS__); }
//#define DEBUG_MASTER(format, ...) { if(tw_ismaster()) CkPrintf("MASTER: "format, ## __VA_ARGS__); }

#endif
