#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

// Included for size_t and int32_t respectively
#include <cstddef>
#include <stdint.h>

// TODO: Make this a more complete listing of types?
// TODO: Should we have a central file for all types, or just define types
// where they are needed.

// Types used in the Charm++ backend of the framework
typedef double Time;
class LPStruct;
class LPType;
class Event;

// API layer linking our types to ROSS types
typedef Time tw_stime;
typedef unsigned long long tw_lpid;
typedef Event tw_event;
typedef LPStruct tw_lp;
typedef LPType tw_lptype;

// These typedefs are similar to function ptr typedefs used by ROSS.
// TODO: Need to figure out how we are doing maps exactly, and how many we need.
// TODO: How much do we need this API to match directly to the ROSS equivalent?
typedef void (*init_f) (LPStruct*);
typedef void (*event_f) (LPStruct*, Event*);
typedef void (*revent_f) (LPStruct*, Event*);
typedef void (*finalize_f) (LPStruct*);
typedef unsigned (*map_f) (tw_lpid);
typedef LPType* (*type_map_f) (tw_lpid);

// Typedefs for rand
typedef int32_t* tw_seed;

// TODO: This should be somewhere else
#define TW_LOC  __FILE__,__LINE__

#endif
