#include "globals.h"

#include "ross_block.h"
#include "scheduler.h"

numlp_map_f g_numlp_map;  // chare -> numlps on that chare
init_map_f  g_init_map;   // (chare x lid) -> gid
type_map_f  g_type_map;   // gid -> type
local_map_f g_local_map;  // gid -> lid
chare_map_f g_chare_map;  // gid -> chare

unsigned g_tw_synchronization_protocol;
unsigned g_tw_gvt_scheme;
unsigned g_tw_expected_events;
tw_stime g_tw_ts_end;       // end time of simulation
unsigned g_tw_mblock;       // number of events per gvt interval
unsigned g_tw_gvt_interval; // number of intervals per gvt
unsigned g_tw_gvt_phases;   // number of phases of the gvt
unsigned g_tw_greedy_start; // whether we allow a greedy start or not
unsigned g_tw_async_reduction; // allow GVT reduction and event exec to overlap
unsigned g_tw_ldb_interval; // number of intervals to wait before ldb
unsigned g_tw_max_ldb;      // max number of times we will load balance
unsigned g_tw_stat_interval;// number of intervals between stat logging
tw_stime g_tw_lookahead;    // event lookahead for conservative
tw_stime g_tw_leash;        // gvt leash for optimistic
double   gvt_print_interval; // determines frequency of progress print outs
tw_seed* g_tw_rng_seed;
size_t   g_tw_rng_max;
unsigned g_tw_nRNG_per_lp;
unsigned g_tw_rng_default;
unsigned g_num_chares;    // number of chares
unsigned g_lps_per_chare; // number of LPs per chare (if constant)
unsigned g_total_lps;     // number of LPs in the simulation
size_t   g_tw_msg_sz;
unsigned g_tw_max_events_buffered;
unsigned g_tw_max_remote_events_buffered;

// Function for setting default/initial values
void clear_globals() {
  g_tw_synchronization_protocol = CONSERVATIVE;
  g_tw_gvt_scheme    = 1;
  g_tw_ts_end        = 100000.0;
  g_tw_mblock        = 16;
  g_tw_gvt_interval  = 16;
  g_tw_gvt_phases    = 0;
  g_tw_greedy_start  = 0;
  g_tw_async_reduction = 0;
  g_tw_ldb_interval  = 0;
  g_tw_stat_interval = 16;
  g_tw_max_ldb       = 0;
  g_tw_lookahead     = 0.005;
  g_tw_leash         = 0.0;
  gvt_print_interval = 1.0;

  g_tw_rng_seed    = NULL;
  g_tw_rng_max     = 1;
  g_tw_nRNG_per_lp = 1;
  g_tw_rng_default = 1;

  g_lps_per_chare  = 1;
  g_num_chares     = 1;
  g_total_lps      = 1;

  g_tw_msg_sz              = 0;
  g_tw_max_events_buffered = 1024;
  g_tw_max_remote_events_buffered = 1024;

  g_numlp_map  = NULL;
  g_init_map   = init_block_map;
  g_type_map   = NULL;
  g_local_map  = local_block_map;
  g_chare_map  = chare_block_map;
}

Globals* get_globals() {
  static Globals* globals = scheduler_proxy.ckLocalBranch()->globals;
  return globals;
}
