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

extern LPMapper*  g_lp_mapper;  ///< mapper for placing and locating LPs
extern LPFactory* g_lp_factory; ///< factory for creating LPs based on global id
///@}

/** \name Event Configuration *////@{
extern Time g_tw_lookahead;           ///< event lookahead (conservative)
extern uint32_t g_num_msg_types;      ///< number of message types
extern uint32_t g_event_buffer_size;  ///< event buffer size
///@}

/** \name Scheduler Configuration *////@{
extern unsigned g_tw_synchronization_protocol;    ///< scheduler type
extern unsigned g_tw_mblock;                      ///< batch size for event execution
extern Time g_tw_ts_end;                      ///< end time of simulation
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
extern unsigned g_tw_ldb_interval;  ///< number of intervals to wait before lb
extern unsigned g_tw_max_ldb;       ///< max number of times to call lb
extern unsigned g_tw_ldb_metric;    ///< metric used to measure LP load
extern unsigned g_tw_metric_ts_abs; ///< use absolute time or not
extern unsigned g_tw_metric_invert; ///< whether or not to invert metric
///@}

/** \name RNG Configuration *////@{
extern unsigned g_tw_rng_default; ///< \todo documentation needed
///@}

/** \name Misc Configuration *////@{
extern unsigned gvt_print_interval;   ///< frequency of progress print outs
extern unsigned g_tw_stat_interval;   ///< frequency of logging stats (in GVTs)
extern long int g_tw_expected_events; ///< expected number of committed events
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
     * Sentinel event signifying some kind of error.
     * \todo Is this the best place to store this?
     */
    Event* abort_event;
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

    Globals() : g_last_gvt(0),
                abort_event(NULL),
                event_buffer(NULL),
                avl_list_head(NULL) {}
};

void clear_globals();   ///< sets all global variables to default values
Globals* get_globals(); ///< returns a pointer to the local Globals object

/** Macro to simplify global variable access */
#define PE_VALUE(x) get_globals()->x

#endif
