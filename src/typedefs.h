#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

// These are types used in ROSS already
typedef double tw_stime;
typedef double Time;
typedef unsigned long long tw_lpid;

class LPStruct;
class Event;

// These typedefs are similar to function ptr typedefs used by ROSS.
// TODO: Need to figure out how we are doing maps exactly, and how many we need.
typedef unsigned (*map_f) (tw_lpid);
typedef void (*event_f) (LPStruct*, Event*);
typedef void (*revent_f) (LPStruct*, Event*);

#endif
