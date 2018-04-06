/** \file globals.C
 *  Definitions for global variables and associated functions.
 *
 *  \todo Many of these variables really belong to specific modules, and can
 *  also be made non-global.
 */

#include "globals.h"

#include "ross_block.h"
#include "scheduler.h"

/** \name LP Configuration
 */
///@{
uint32_t g_total_lps;
uint32_t g_num_chares;
uint32_t g_lps_per_chare;

numlp_map_f g_numlp_map;
init_map_f  g_init_map;
type_map_f  g_type_map;
local_map_f g_local_map;
chare_map_f g_chare_map;
///@}

/** \name Event Configuration */
///@{
tw_stime g_tw_lookahead;
size_t   g_tw_msg_sz;
uint32_t g_tw_max_events_buffered;
uint32_t g_tw_max_remote_events_buffered;
///@}

/** \name Scheduler Configuration */
///@{
uint32_t g_tw_synchronization_protocol;
uint32_t g_tw_mblock;
tw_stime g_tw_ts_end;
///@}

/** \name GVT Configuration */
///@{
uint32_t g_tw_gvt_scheme;
uint32_t g_tw_gvt_interval;
uint32_t g_tw_gvt_trigger;
uint32_t g_tw_gvt_phases;
uint32_t g_tw_gvt_bucket_size;
uint32_t g_tw_async_reduction;
double   g_tw_reserve_threshold;
uint32_t g_tw_reserve_buckets;
uint32_t g_tw_adaptive_buckets;
uint32_t g_tw_clear_lag;
uint32_t g_tw_clear_buckets;
///@}

/** \name LB Configuration */
///@{
uint32_t g_tw_ldb_first;
uint32_t g_tw_ldb_interval;
uint32_t g_tw_max_ldb;
uint32_t g_tw_ldb_metric;
uint32_t g_tw_metric_ts_abs;
uint32_t g_tw_metric_invert;
///@}

/** \name RNG Configuration */
///@{
tw_seed* g_tw_rng_seed;
size_t   g_tw_rng_max;
uint32_t g_tw_nRNG_per_lp;
uint32_t g_tw_rng_default;
///@}

/** \name Misc Configuration */
///@{
int32_t  g_tw_expected_events;
uint32_t gvt_print_interval;
uint32_t g_tw_stat_interval;
///@}

/**
 * \todo Can these just be initialized at declaration? Especially if they are
 * all moved to more appropriate modules.
 */
void clear_globals() {
  g_total_lps      = 1;
  g_num_chares     = 1;
  g_lps_per_chare  = 1;

  g_numlp_map  = NULL;
  g_init_map   = init_block_map;
  g_type_map   = NULL;
  g_local_map  = local_block_map;
  g_chare_map  = chare_block_map;

  g_tw_lookahead                  = 0.005;
  g_tw_msg_sz                     = 0;
  g_tw_max_events_buffered        = 1024;
  g_tw_max_remote_events_buffered = 1024;

  g_tw_synchronization_protocol = CONSERVATIVE;
  g_tw_mblock                   = 16;
  g_tw_ts_end                   = 100000.0;

  g_tw_gvt_scheme       = 1;
  g_tw_gvt_interval     = 16;
  g_tw_gvt_trigger      = 1;
  g_tw_gvt_phases       = 2;
  g_tw_gvt_bucket_size  = 8;
  g_tw_async_reduction  = 0;
  g_tw_reserve_threshold = 1.0;
  g_tw_reserve_buckets = 0;
  g_tw_adaptive_buckets = 0;
  g_tw_clear_lag = 0;
  g_tw_clear_buckets = 1;

  g_tw_ldb_first     = 0;
  g_tw_ldb_interval  = 0;
  g_tw_max_ldb       = 0;
  g_tw_ldb_metric    = 0;
  g_tw_metric_ts_abs = 0;
  g_tw_metric_invert = 0;

  g_tw_rng_seed    = NULL;
  g_tw_rng_max     = 1;
  g_tw_nRNG_per_lp = 1;
  g_tw_rng_default = 1;

  gvt_print_interval = 64;
  g_tw_stat_interval = 0;
  g_tw_expected_events = -1;
}

Globals* get_globals() {
  /** Store the pointer in a static variable for faster lookup */
  static Globals* globals = ((Scheduler*)CkLocalBranch(scheduler_id))->globals;
  return globals;
}
