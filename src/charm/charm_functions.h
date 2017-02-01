#ifndef _CHARM_FUNCTIONS_H_
#define _CHARM_FUNCTIONS_H_

void charm_init(int argc, char** argv);
void charm_run();
void charm_exit();

int tw_ismaster();
int tw_nnodes();

int tw_mype();
void tw_abort(const char*);

#define USER_EVENT_FWD    1001
#define USER_EVENT_RB     1002
#define USER_EVENT_CANCEL 1003
#define USER_EVENT_GVT    1004
#define USER_EVENT_LDB    1005
#define USER_EVENT_FC     1006
#define USER_STAT_MEMORY_USAGE 1007
#define USER_STAT_EVENTS_COMMITTED 1008
#define USER_STAT_ROLLED_BACK 1009
#define USER_STAT_EXECUTED 1010

#ifdef CMK_TRACE_ENABLED
#define BRACKET_TRACE(code,event) \
double start = CmiWallTimer(), end; \
code \
end = CmiWallTimer(); \
traceUserBracketEvent(event, start, end);
#else
#define BRACKET_TRACE(code, event) code
#endif

#ifdef DEBUG_ON
#define DEBUG(format, ...) { CkPrintf("DEBUG: " format, ## __VA_ARGS__); }
#else
#define DEBUG(format, ...) { }
#endif

#ifdef DEBUG_MASTER_ON
#define DEBUG_MASTER(format, ...) { if(tw_ismaster()) CkPrintf("MASTER: " format, ## __VA_ARGS__); }
#else
#define DEBUG_MASTER(format, ...) { }
#endif

#ifdef DEBUG_LP_ON
#define DEBUG_LP(format, ...) { CkPrintf("LP[%d]: " format, thisIndex, ## __VA_ARGS__); }
#else
#define DEBUG_LP(format, ...) {}
#endif

#ifdef DEBUG_PE_ON
#define DEBUG_PE(format, ...) { CkPrintf("PE[%d] " format, CkMyPe(), ## __VA_ARGS__); }
#else
#define DEBUG_PE(format, ...) {}
#endif

#endif
