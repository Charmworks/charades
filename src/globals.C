/** \file globals.C
 *  Definitions for global variables and associated functions.
 *
 *  \todo Many of these variables really belong to specific modules, and can
 *  also be made non-global.
 */

#include "globals.h"
#include "scheduler.h"

/** \name LP Configuration
 */
///@{
unsigned g_total_lps;
unsigned g_num_chares;
unsigned g_lps_per_chare;

LPMapper*   g_lp_mapper;
LPFactory*  g_lp_factory;
///@}

/** \name Event Configuration */
///@{
Time g_tw_lookahead;
uint32_t g_num_msg_types;
uint32_t g_event_buffer_size;
///@}

/** \name Scheduler Configuration */
///@{
unsigned g_tw_synchronization_protocol;
unsigned g_tw_mblock;
Time g_tw_ts_end;
///@}

/** \name GVT Configuration */
///@{
unsigned g_tw_gvt_scheme;
unsigned g_tw_gvt_interval;
unsigned g_tw_gvt_trigger;
unsigned g_tw_gvt_phases;
unsigned g_tw_gvt_bucket_size;
unsigned g_tw_async_reduction;
double   g_tw_reserve_threshold;
uint32_t g_tw_reserve_buckets;
uint32_t g_tw_adaptive_buckets;
uint32_t g_tw_clear_lag;
uint32_t g_tw_clear_buckets;
///@}

/** \name LB Configuration */
///@{
uint32_t g_tw_ldb_continuous;
uint32_t g_tw_ldb_first;
unsigned g_tw_ldb_interval;
unsigned g_tw_max_ldb;
unsigned g_tw_ldb_metric;
unsigned g_tw_metric_ts_abs;
unsigned g_tw_metric_invert;
///@}

/** \name RNG Configuration */
///@{
unsigned g_tw_rng_default;
///@}

/** \name Misc Configuration */
///@{
char g_output_dir[256];
unsigned gvt_print_interval;
unsigned g_tw_stat_interval;
long int g_tw_expected_events;
///@}

/**
 * \todo Can these just be initialized at declaration? Especially if they are
 * all moved to more appropriate modules.
 */
void clear_globals() {
  g_tw_synchronization_protocol = CONSERVATIVE;
  g_tw_gvt_scheme    = 1;
  g_tw_ts_end        = 1024000;
  g_tw_mblock        = 16;
  g_tw_gvt_interval  = 16;
  g_tw_gvt_trigger   = 1;
  g_tw_gvt_phases    = 2;
  g_tw_gvt_bucket_size    = 8;
  g_tw_async_reduction = 0;
  g_tw_reserve_threshold = 1.0;
  g_tw_reserve_buckets = 0;
  g_tw_adaptive_buckets = 0;
  g_tw_clear_lag = 0;
  g_tw_clear_buckets = 1;
  g_tw_ldb_continuous = 0;
  g_tw_ldb_first = 0;
  g_tw_ldb_interval  = 0;
  g_tw_stat_interval = 16;
  g_tw_max_ldb       = 0;
  g_tw_ldb_metric    = 0;
  g_tw_metric_ts_abs = 0;
  g_tw_metric_invert = 0;
  g_tw_lookahead     = 5;
  gvt_print_interval = 64;
  g_tw_expected_events = -1;

  g_tw_rng_default = 1;

  g_lps_per_chare  = 1;
  g_num_chares     = 1;
  g_total_lps      = 1;

  g_num_msg_types     = 0;
  g_event_buffer_size = 8192;

  g_lp_mapper   = NULL;
  g_lp_factory  = NULL;
}

Globals* get_globals() {
  /** Store the pointer in a static variable for faster lookup */
  return CpvAccess(g_scheduler)->globals;
}

#if not USE_CHARMC
// TODO: Maybe these should be moved somewhere else?
void _registerExternalModules(char **argv) {}
void _createTraces(char **argv) {}
#endif
