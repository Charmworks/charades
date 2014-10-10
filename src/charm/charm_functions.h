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
#define DEBUG2(format, ...) { }
#define DEBUG3(format, ...) { }
#define DEBUG4(format, ...) { }
//#define DEBUG(format, ...) { CkPrintf("[%d] "format, CkMyPe(), ## __VA_ARGS__); }
//#define DEBUG2(format, ...) { CkPrintf("[%d] "format, CkMyPe(), ## __VA_ARGS__); }
//#define DEBUG3(format, ...) { CkPrintf("[%d] "format, CkMyPe(), ## __VA_ARGS__); }
//#define DEBUG4(format, ...) { CkPrintf("[%d] "format, CkMyPe(), ## __VA_ARGS__); }

#endif
