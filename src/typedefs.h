#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

typedef double tw_stime;

typedef unsigned long long tw_lpid;

class LPData;

typedef int (*map_f) (tw_lpid);
typedef unsigned (*map_local_f) (tw_lpid);
typedef void (*event_f) (LPData*, Event*);
typedef void (*revent_f) (LPData*, Event*);

#endif
