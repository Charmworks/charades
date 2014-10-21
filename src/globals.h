#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "typedefs.h"
#include "ross_event.h"
#include "event_buffer.h"

// Included for FILE
#include <stdio.h>

// A struct for holding global variables used by ROSS. An instance of this
// struct will be held by each PE group chare.
// TODO (eric): Organize the declarations by module and get rid of unecessary globals
// TODO (eric): Make sure these have the right types
struct Globals {
  unsigned g_lps_per_chare; //number of LPs per chare
  unsigned g_tw_synchronization_protocol;
  tw_stime g_tw_ts_end; //end time of simulation
  unsigned g_tw_mblock; //batchSize
  unsigned g_tw_gvt_interval; //frequency of gvt
  unsigned g_tw_events_per_pe_extra;

  unsigned g_tw_nlp; //number of LP per PE
  size_t g_tw_memory_sz; //TBD

  // RNG Globals
  tw_seed* g_tw_rng_seed;
  size_t g_tw_rng_max;
  unsigned g_tw_nRNG_per_lp;
  unsigned g_tw_rng_default;

  FILE* g_tw_csv;

  unsigned g_num_lp_chares; //number of LP chares

  // Globals used in events
  EventBuffer* event_buffer;
  size_t g_tw_msg_sz;
  unsigned g_tw_max_events_buffered;
  tw_stime g_tw_min_detected_offset;
  tw_event* abort_event;
  tw_stime g_tw_lookahead;

  // Global mapping function pointers
  init_map_f g_init_map;
  type_map_f g_type_map;
  // TODO (eric): This map may be stored in the type instead
  local_map_f g_local_map;

  AvlTree avl_list_head;
  tw_out* output;
  tw_stime lastGVT;
  double netEvents;
  double total_time;
};

// Defined in pe.C
Globals* get_globals();

#define PE_VALUE(x) get_globals()->x

#endif
