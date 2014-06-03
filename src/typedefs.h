#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

// Types used in the Charm++ backend of the framework
typedef double Time;
class LPStruct;
class Event;

// API layer linking our types to ROSS types
typedef Time tw_stime;
typedef unsigned long long tw_lpid;
typedef Event tw_event;
typedef LPStruct tw_lp;

// These typedefs are similar to function ptr typedefs used by ROSS.
// TODO: Need to figure out how we are doing maps exactly, and how many we need.
// TODO: How much do we need this API to match directly to the ROSS equivalent?
typedef void (*init_f) (LPStruct*);
typedef void (*event_f) (LPStruct*, Event*);
typedef void (*revent_f) (LPStruct*, Event*);
typedef void (*finalize_f) (LPStruct*);
typedef unsigned (*map_f) (tw_lpid);

#endif
