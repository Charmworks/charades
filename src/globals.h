/** \file globals.h
 *  Declarations for global variables and associated functions.
 *
 *  Most declarations in this file are for "constant" global variables that are
 *  set during startup based on model/simulator configuration and don't ever
 *  change. Variables that are global per PE, but will change at a PE level are
 *  wrapped in a small class which will be instantiated once on each PE.
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "typedefs.h"

/** \name LP Configuration *////@{
extern unsigned g_total_lps;     ///< total number of LPs
extern unsigned g_num_chares;    ///< total number of chares
extern unsigned g_lps_per_chare; ///< number of LPs per chare (if constant)

extern numlp_map_f g_numlp_map;  ///< map from chare id to number of lps on that chare
extern init_map_f  g_init_map;   ///< map from chare id and local lp id to global lp id
extern type_map_f  g_type_map;   ///< map from global lp id to lp type
extern local_map_f g_local_map;  ///< map from global lp id to local lp id
extern chare_map_f g_chare_map;  ///< map from global lp id to chare id
///@}

/** \name Event Configuration *////@{
extern tw_stime g_tw_lookahead;                   ///< event lookahead (conservative)
extern size_t   g_tw_msg_sz;                      ///< user msg size
extern unsigned g_tw_max_events_buffered;         ///< event buffer size
extern unsigned g_tw_max_remote_events_buffered;  ///< remote event buffer size
///@}

/** \name Scheduler Configuration *////@{
extern unsigned g_tw_synchronization_protocol;    ///< scheduler type
extern unsigned g_tw_mblock;                      ///< batch size for event execution
extern tw_stime g_tw_ts_end;                      ///< end time of simulation
///@}

/** \name GVT Configuration *////@{
extern unsigned g_tw_gvt_scheme;      ///< GVT algorithm
extern unsigned g_tw_gvt_interval;    ///< distance between GVT calls
extern unsigned g_tw_gvt_trigger;     ///< unit of the GVT interval (time or events)
extern unsigned g_tw_gvt_phases;      ///< number of phases in a phased gvt
extern unsigned g_tw_gvt_bucket_size; ///< size of each bucket in a bucketed gvt
extern unsigned g_tw_async_reduction; ///< use asynchronous reductions in GVTs
///@}

/** \name LB Configuration *////@{
extern unsigned g_tw_ldb_first;     ///< first iteration to call load balancing
extern unsigned g_tw_ldb_interval;  ///< number of intervals to wait before lb
extern unsigned g_tw_max_ldb;       ///< max number of times to call lb
extern unsigned g_tw_ldb_metric;    ///< metric used to measure LP load
extern unsigned g_tw_metric_ts_abs; ///< use absolute time or not
extern unsigned g_tw_metric_invert; ///< whether or not to invert metric
///@}

/** \name RNG Configuration *////@{
extern tw_seed* g_tw_rng_seed;    ///< initial seed for all RNGs
extern size_t   g_tw_rng_max;     ///< \todo documentation needed
extern unsigned g_tw_nRNG_per_lp; ///< number of RNGs per LP
extern unsigned g_tw_rng_default; ///< \todo documentation needed
///@}

/** \name Misc Configuration *////@{
extern long int g_tw_expected_events; ///< expected number of committed events
extern unsigned gvt_print_interval;   ///< frequency of progress print outs
extern unsigned g_tw_stat_interval;   ///< frequency of logging stats (in GVTs)
///@}

class EventBuffer;

/**
 * PE-level variables which may change over time
 */
class Globals {
  public:
    /**
     * Last computed GVT, used for rollbacks.
     * \todo This should be moved to GVT base class probably
     */
    Time g_last_gvt;
    /**
     * Start of the virtual time leash when using virtual time as a GVT trigger.
     * When a GVT is completed this gets set as that time. During the GVT
     * computation it is set as the time this PE contributed. The only time this
     * will actually come into effect is with asynchronous GVTs.
     */
    Time g_leash_time;
    /**
     * Sentinel event signifying some kind of error.
     * \todo Is this the best place to store this?
     */
    tw_event* abort_event;
    /**
     * Buffer of pre-allocated events for entire simulation.
     * \todo Can probably be moved to scheduler, which will handle event
     * allocation and management.
     */
    EventBuffer* event_buffer;
    /**
     * Hash table for events received remotely in optimistic execution.
     * \todo Can this go in the LP class for smaller hash tables?
     * \todo Can this be changed to an unordered_map?
     */
    AvlTree avl_list_head;

    Globals() : g_last_gvt(0.0), g_leash_time(0.0),
        abort_event(NULL), event_buffer(NULL), avl_list_head(NULL) {}
};

void clear_globals();   ///< sets all global variables to default values
Globals* get_globals(); ///< returns a pointer to the local Globals object

/** Macro to simplify global variable access */
#define PE_VALUE(x) get_globals()->x

#endif
