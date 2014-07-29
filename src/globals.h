#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "typedefs.h"

// Included for FILE
#include <stdio.h>

// A struct for holding global variables used by ROSS. An instance of this
// struct will be held by each PE group chare.
// TODO: Organize the declarations by module and get rid of unecessary globals
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

  FILE* g_tw_csv;
};

// Get the local branch of the PE group and return its globals.
// If possible we should cache the pointer to the local branch.
Globals* get_globals();

#endif
