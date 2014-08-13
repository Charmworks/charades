#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "typedefs.h"

// Included for FILE
#include <stdio.h>

// A struct for holding global variables used by ROSS. An instance of this
// struct will be held by each PE group chare.
// TODO: Organize the declarations by module and get rid of unecessary globals
// TODO: Initialization of defaultst 
struct Globals {
  unsigned g_lps_per_chare;
  unsigned g_tw_synchronization_protocol;
  tw_stime g_tw_ts_end;
  unsigned g_tw_mblock;
  unsigned g_tw_events_per_pe_extra;

  unsigned g_tw_nlp;
  size_t g_tw_memory_sz;
  size_t g_tw_msg_sz;

  tw_seed* g_tw_rng_seed;
  size_t g_tw_rng_max;

  FILE* g_tw_csv;

  unsigned g_num_lp_chares;

  // Globals used in events
  // TODO: Make sure these have the right types
  size_t g_tw_user_data_size;
  unsigned g_tw_max_events_buffered;
  unsigned g_tw_min_detected_offset;
  tw_event* abort_event;
  tw_stime g_tw_lookahead;

};

// Functions for modifying globals including readonly chare proxies.
// These functions are defined in the appropriate Charm++ backend files.
void create_pes();
void create_lps();

// Get the local branch of the PE group and return its globals.
// If possible we should cache the pointer to the local branch.
Globals* get_globals();

#define PE_VALUE(x) get_globals()->x

CkpvExtern(AvlTree, avl_list_head);

#endif
