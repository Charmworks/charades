#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

// These are types used in ROSS already
typedef double tw_stime;
typedef double Time;
typedef unsigned long long tw_lpid;

class LPData;
class Event;

// These typedefs are similar to function ptr typedefs used by ROSS.
typedef int (*map_f) (tw_lpid);
typedef unsigned (*map_local_f) (tw_lpid);
typedef void (*event_f) (LPData*, Event*);
typedef void (*revent_f) (LPData*, Event*);

#endif
