#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

// Included for size_t and int32_t respectively
#include <stdlib.h>
#include <stdint.h>

// TODO: Make this a more complete listing of types?
// TODO: Should we have a central file for all types, or just define types
// where they are needed.

/**
 * Synchronization protocol used
 */
enum tw_synch_e {
    NO_SYNCH,
    SEQUENTIAL,
    CONSERVATIVE,
    OPTIMISTIC,
};

struct MemUsage {
  unsigned long long max_memory;
  double avg_memory;
};

// Types used in the Charm++ backend of the framework
typedef double Time;

class LPStruct;
class LPType;
class LP;
class Event;
class ProcessedQueue;
class PendingQueue;


typedef unsigned long long EventID;
//typedef enum tw_event_owner tw_event_owner;
typedef unsigned long long tw_lpid;
typedef unsigned long long tw_peid;
typedef struct avlNode *AvlTree;
typedef struct tw_bf tw_bf;

// API layer linking our types to ROSS types
typedef Time tw_stime;
typedef EventID tw_eventid;
typedef Event tw_event;
typedef LPStruct tw_lp;
typedef LPType tw_lptype;

// Statistics
typedef unsigned long long tw_stat;
typedef uint64_t tw_clock;

// TODO: This doesn't really make sense;
typedef LP tw_pe;

// These typedefs are similar to function ptr typedefs used by ROSS.
// TODO: Need to figure out how we are doing maps exactly, and how many we need.
// TODO: How much do we need this API to match directly to the ROSS equivalent?
typedef void (*init_f) (void*,LPStruct*);
typedef void (*event_f) (void*, tw_bf*, char*, LPStruct*);
typedef void (*revent_f) (void*, tw_bf*, char*, LPStruct*);
typedef void (*final_f) (void*, LPStruct*);
typedef void (*commit_f) (void*, tw_bf*, char*, LPStruct*);

// This map takes an LP chare index, and a local lpid and returns a global id.
typedef tw_lpid (*init_map_f) (unsigned, tw_lpid);
// This map takes a global lpid and returns the chare index containing that lp.
typedef unsigned (*chare_map_f) (tw_lpid);
// This map takes an LP global id, and returns the local id.
typedef tw_lpid (*local_map_f) (tw_lpid);
// This map takes an LP chare index and returns how many LPs are on that chare.
typedef unsigned (*numlp_map_f) (unsigned);

// This map takes a global lpid and returns the type of that lp.
// Will be defined in the model.
typedef LPType* (*type_map_f) (tw_lpid);

// Typedefs for rand
typedef int32_t* tw_seed;

// TODO: This should be somewhere else
#define TW_LOC  __FILE__,__LINE__

#endif
