#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "typedefs.h"
#include "ross_block.h"
#include "ross_event.h"
#include "event_buffer.h"

#include <stdio.h> // Included for FILE
#include <float.h> // Included for DBL_MAX

extern numlp_map_f g_numlp_map;  // chare -> numlps on that chare
extern init_map_f  g_init_map;   // (chare x lid) -> gid
extern type_map_f  g_type_map;   // gid -> type
extern local_map_f g_local_map;  // gid -> lid
extern chare_map_f g_chare_map;  // gid -> chare
extern unsigned g_tw_synchronization_protocol;
extern tw_stime g_tw_ts_end;       // end time of simulation
extern unsigned g_tw_mblock;       // number of events per gvt interval
extern unsigned g_tw_gvt_interval; // number of intervals per gvt
extern unsigned g_tw_gvt_phases;   // number of phases in the gvt
extern unsigned g_tw_greedy_start; // whether or not we greedily start the gvt
extern unsigned g_tw_ldb_interval; // number of intervals to wait before ldb
extern tw_stime g_tw_lookahead;    // event lookahead for conservative
extern tw_stime g_tw_leash;        // GVT leash for optimistic
extern double  gvt_print_interval; // determines frequency of progress print outs
extern size_t    g_tw_rng_max;
extern unsigned  g_tw_nRNG_per_lp;
extern unsigned  g_tw_rng_default;
extern unsigned g_num_chares;    // number of chares
extern unsigned g_lps_per_chare; // number of LPs per chare (if constant)
extern unsigned g_total_lps;     // number of LPs in the simulation
extern size_t        g_tw_msg_sz;
extern unsigned      g_tw_max_events_buffered;

// A struct for holding global variables used by ROSS. An instance of this
// struct will be held by each PE group chare.
struct Globals {
  // Not read onlies
  tw_stime g_last_gvt;        // used for rollback purposes
  double  percent_complete;   // current progress through simulation

  // Differs by PE
  tw_seed*  g_tw_rng_seed;
  EventBuffer*  event_buffer;
  tw_event*     abort_event;
  FILE*   g_tw_csv;
  AvlTree avl_list_head;
  tw_out* output;
};

// Function for setting default/initial values
inline void initialize_globals(Globals* globals) {
  g_tw_synchronization_protocol = CONSERVATIVE;
  g_tw_ts_end        = 100000.0;
  g_tw_mblock        = 16;
  g_tw_gvt_interval  = 16;
  g_tw_gvt_phases    = 0;
  g_tw_greedy_start  = 0;
  g_tw_ldb_interval  = 0;
  g_tw_lookahead     = 0.005;
  g_tw_leash         = 0.0;
  gvt_print_interval = 1.0;

  g_tw_rng_max     = 1;
  g_tw_nRNG_per_lp = 1;
  g_tw_rng_default = 1;

  g_lps_per_chare  = 1;
  g_num_chares     = 1;
  g_total_lps      = 1;

  g_tw_msg_sz              = 0;
  g_tw_max_events_buffered = 1024;

  g_numlp_map  = NULL;
  g_init_map   = init_block_map;
  g_type_map   = NULL;
  g_local_map  = local_block_map;
  g_chare_map  = chare_block_map;

  globals->g_last_gvt         = 0.0;
  globals->percent_complete   = 0.0;
  globals->g_tw_rng_seed    = NULL;
  globals->event_buffer             = NULL;
  globals->abort_event              = NULL;
  globals->g_tw_csv = NULL;
}

// Defined in pe.C
Globals* get_globals();

#define PE_VALUE(x) get_globals()->x
#define ROSS_CONSTANT(x) PE_VALUE(x)

#endif
