#ifndef _CHARM_FUNCTIONS_H_
#define _CHARM_FUNCTIONS_H_

void create_lps();
void init_lps();
void charm_init(int argc, char** argv);
void charm_run();
void charm_exit();

int tw_ismaster();
int tw_nnodes();

Event* tw_current_event(tw_lp*);
int tw_mype();
void tw_abort(const char*);

void charm_event_send(int, tw_event*);
void charm_anti_send(tw_event*);
void charm_add_to_cancel_q(tw_event*);
tw_stime tw_now(tw_lp*);
void charm_delete_pending(tw_event*);

#define DEBUG(format, ...) { }
#define DEBUG2(format, ...) { }
#define DEBUG3(format, ...) { }
#define DEBUG4(format, ...) { }
//#define DEBUG(format, ...) { CkPrintf("[%d] "format, CkMyPe(), ## __VA_ARGS__); }
//#define DEBUG2(format, ...) { CkPrintf("[%d] "format, CkMyPe(), ## __VA_ARGS__); }
//#define DEBUG3(format, ...) { CkPrintf("[%d] "format, CkMyPe(), ## __VA_ARGS__); }
//#define DEBUG4(format, ...) { CkPrintf("[%d] "format, CkMyPe(), ## __VA_ARGS__); }

#endif
