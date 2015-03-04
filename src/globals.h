#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "typedefs.h"
#include "ross_block.h"
#include "ross_event.h"
#include "event_buffer.h"

#include <stdio.h> // Included for FILE
#include <float.h> // Included for DBL_MAX

// A struct for holding global variables used by ROSS. An instance of this
// struct will be held by each PE group chare.
struct Globals {
  // Scheduler globals
  unsigned g_tw_synchronization_protocol;
  tw_stime g_tw_ts_end;       // end time of simulation
  unsigned g_tw_mblock;       // number of events per gvt interval
  unsigned g_tw_gvt_interval; // number of intervals per gvt
  tw_stime g_tw_lookahead;    // event lookahead for conservative
  tw_stime g_last_gvt;        // used for rollback purposes
  double  gvt_print_interval; // determines frequency of progress print outs
  double  percent_complete;   // current progress through simulation

  // RNG Globals
  tw_seed*  g_tw_rng_seed;
  size_t    g_tw_rng_max;
  unsigned  g_tw_nRNG_per_lp;
  unsigned  g_tw_rng_default;

  // LP/Chare globals
  unsigned g_num_chares;    // number of chares
  unsigned g_lps_per_chare; // number of LPs per chare (if constant)
  unsigned g_total_lps;     // number of LPs in the simulation

  // Events globals
  EventBuffer*  event_buffer;
  size_t        g_tw_msg_sz;
  unsigned      g_tw_max_events_buffered;
  tw_event*     abort_event;

  // Mapping globals
  numlp_map_f g_numlp_map;  // chare -> numlps on that chare
  init_map_f  g_init_map;   // (chare x lid) -> gid
  type_map_f  g_type_map;   // gid -> type
  local_map_f g_local_map;  // gid -> lid
  chare_map_f g_chare_map;  // gid -> chare

  // Misc globals
  FILE*   g_tw_csv;
  AvlTree avl_list_head;
  tw_out* output;
};

// Function for setting default/initial values
inline void initialize_globals(Globals* globals) {
  globals->g_tw_synchronization_protocol = CONSERVATIVE;
  globals->g_tw_ts_end        = 1024;
  globals->g_tw_mblock        = 16;
  globals->g_tw_gvt_interval  = 16;
  globals->g_tw_lookahead     = .005;
  globals->g_last_gvt         = 0.0;
  globals->gvt_print_interval = 1.0;
  globals->percent_complete   = 0.0;

  globals->g_tw_rng_seed    = NULL;
  globals->g_tw_rng_max     = 1;
  globals->g_tw_nRNG_per_lp = 1;
  globals->g_tw_rng_default = 1;

  globals->g_lps_per_chare  = 1;
  globals->g_num_chares     = 1;
  globals->g_total_lps      = 1;

  globals->event_buffer             = NULL;
  globals->g_tw_msg_sz              = 0;
  globals->g_tw_max_events_buffered = 1024;
  globals->abort_event              = NULL;

  globals->g_numlp_map  = NULL;
  globals->g_init_map   = init_block_map;
  globals->g_type_map   = NULL;
  globals->g_local_map  = local_block_map;
  globals->g_chare_map  = chare_block_map;

  globals->g_tw_csv = NULL;
}

// Defined in pe.C
Globals* get_globals();

#define PE_VALUE(x) get_globals()->x
#define ROSS_CONSTANT(x) PE_VALUE(x)

#endif
