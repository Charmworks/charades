#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

typedef double Time;

typedef unsigned long long tw_lpid;

typedef int (*map_f) (tw_lpid);
typedef unsigned (*map_local_f) (tw_lpid);
typedef void (*event_f) (Event*);

#endif
