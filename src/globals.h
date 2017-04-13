#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "typedefs.h"

extern numlp_map_f g_numlp_map;  // chare -> numlps on that chare
extern init_map_f  g_init_map;   // (chare x lid) -> gid
extern type_map_f  g_type_map;   // gid -> type
extern local_map_f g_local_map;  // gid -> lid
extern chare_map_f g_chare_map;  // gid -> chare

extern unsigned g_tw_synchronization_protocol;
extern unsigned g_tw_gvt_scheme;
extern unsigned g_tw_expected_events;
extern tw_stime g_tw_ts_end;       // end time of simulation
extern unsigned g_tw_mblock;       // number of events per gvt interval
extern unsigned g_tw_gvt_interval; // number of intervals per gvt
extern unsigned g_tw_gvt_trigger;  // GVT trigger type
extern unsigned g_tw_gvt_phases;   // number of phases in the gvt
extern unsigned g_tw_async_reduction; // allow GVT rdn and event exec overlap
extern unsigned g_tw_ldb_interval; // number of intervals to wait before ldb
extern unsigned g_tw_max_ldb;      // number of intervals to wait before ldb
extern unsigned g_tw_ldb_metric;   // Metric used to measure LP load
extern unsigned g_tw_metric_ts_abs;// Use absolute time or not
extern unsigned g_tw_metric_invert;// Whether or not to invert metric
extern unsigned g_tw_stat_interval;// number of gvts between logging stats
extern tw_stime g_tw_lookahead;    // event lookahead for conservative
extern unsigned gvt_print_interval;// determines frequency of progress print outs
extern tw_seed* g_tw_rng_seed;
extern size_t   g_tw_rng_max;
extern unsigned g_tw_nRNG_per_lp;
extern unsigned g_tw_rng_default;
extern unsigned g_num_chares;    // number of chares
extern unsigned g_lps_per_chare; // number of LPs per chare (if constant)
extern unsigned g_total_lps;     // number of LPs in the simulation
extern size_t   g_tw_msg_sz;
extern unsigned g_tw_max_events_buffered;
extern unsigned g_tw_max_remote_events_buffered;

class EventBuffer;
// A struct for holding global variables used by ROSS. An instance of this
// struct will be held by each PE group chare.
class Globals {
  public:
    Time g_last_gvt;            // TODO: Needed for rollbacks, but should be moved
    // Differs by PE
    tw_event*     abort_event;  // TODO: Can this just be part of event buffer? Do we need this at all even?
    EventBuffer*  event_buffer; // TODO: Move to scheduler
    AvlTree       avl_list_head;      // TODO: Move to optimistic scheduler or maybe even LP, experiment with unordered_map

    Globals() : abort_event(NULL), event_buffer(NULL), avl_list_head(NULL) {}
};

void clear_globals();
Globals* get_globals();

#define PE_VALUE(x) get_globals()->x

#endif
