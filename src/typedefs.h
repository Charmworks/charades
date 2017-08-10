/** \file typedefs.h
 *  Declares most types used within the simulator and by models.
 *
 *  Many of these types are just renaming builtin or existing types, or aliasing
 *  other types. Needs a lot of cleanup. Some types are not used, or are
 *  misleading since the conversion of ROSS.
 *
 *  \todo Cleanup unneeded types
 *  \todo Move type declarations to appropriate modules as needed
 *  \todo Maybe make a seperate file for aliases that is only needed when using
 *  the old ROSS API.
 */

#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

// Included for size_t and int32_t respectively
#include <stdlib.h>
#include <stdint.h>

/** Enum for synchroniaztion protocol. \todo move to scheduler */
enum tw_synch_e {
    NO_SYNCH,
    SEQUENTIAL,
    CONSERVATIVE,
    OPTIMISTIC,
};

/** Struct for recording memory usage. \todo is this even used? */
struct MemUsage {
  unsigned long long max_memory;
  double avg_memory;
};

/** \name Original ROSS typedefs
 *  These are typedefs from original ROSS.
 *  \todo some are not needed anymore since they just typedef structs
 *////@{
typedef uint64_t Time;
typedef uint64_t EventID;         ///< Unique ID for events sent from one LP
typedef uint64_t LPID;            ///< LP ID, either global or local
typedef struct avlNode* AvlTree;  ///< Node in the PE level AVL tree
typedef int32_t* tw_seed;         ///< Type of the RNG seed use by all RNGs
///@}

#define TIME_MAX UINT64_MAX

/** \name Charm++ Types/Forward Declarations
 *  Most of these types were introduced for the Charm++ side, and have aliases
 *  to keep compatible with old ROSS.
 *  \see LPStruct
 *  \see LPType
 *  \see LP
 *  \see Event
 *  \see ProcessedQueue
 *  \see PendingQueue
 *  \todo Many of these forward declarations may not even be needed
 *////@{
class LPBase;
class Event;
class ProcessedQueue;
class PendingQueue;
///@}

/** Map that takes an LP chare index and local LP id and returns a global id */
typedef LPID (*init_map_f) (unsigned, LPID);
/** Map that takes a global LP id and returns the chare where that LP resides */
typedef unsigned (*chare_map_f) (LPID);
/** Map that takes a global LP id and returns a local id within its chare */
typedef LPID (*local_map_f) (LPID);
/** Map that takes a chare index and returns the number of LPs on that chare */
typedef unsigned (*numlp_map_f) (unsigned);

/** Map that takes a global LP id and returns a pointer to the LP type */
typedef LPBase* (*type_map_f) (LPID);
///@}

/** Macro for debug printing \todo should be removed or moved elsewhere */
#define TW_LOC  __FILE__,__LINE__

#endif
