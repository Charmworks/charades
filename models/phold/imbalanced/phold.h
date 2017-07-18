#ifndef INC_phold_h
#define INC_phold_h

#include "ross_api.h"

typedef struct phold_state {
  int dummy_state;
  tw_stime work_load;
  tw_stime mean_delay;
  tw_stime percent_remote;
} phold_state;

typedef struct phold_message {
  tw_stime work_load;
  tw_stime mean_delay;
  tw_stime percent_remote;
} phold_message;

	/*
	 * PHOLD Globals
	 */
static int start_events = 1;

static int load_map = 0;
static tw_stime percent_heavy = 0.0;
static int light_load = 0;
static int heavy_load = 0;
static int load_seed = 0;

static int delay_map = 0;
static tw_stime percent_long = 0.0;
static tw_stime short_delay = 1.0;
static tw_stime long_delay = 5.0;
static int delay_seed = 0;

static int remote_map = 0;
static tw_stime percent_greedy = 0.0;
static tw_stime generous_remote = 0.1;
static tw_stime greedy_remote = 0.0;
static int remote_seed = 0;

static int region_size = 0;

int (*lp_load_map)(tw_lpid);
tw_stime (*lp_delay_map)(tw_lpid);
tw_stime (*lp_remote_map)(tw_lpid);

// Maps for delay:
// Uniform is the default, and has every lp use the short delay mean
// Blocked has a contiguous block of lps use the long delay
// Linear has lps using delay proportional to their lpid
inline tw_stime uniform_lp_delay(tw_lpid lp) {
  return short_delay;
}

inline tw_stime blocked_lp_delay(tw_lpid lp) {
  if (lp >= delay_seed  && lp < delay_seed + g_total_lps*percent_long) {
    return long_delay;
  } else {
    return short_delay;
  }
}

inline tw_stime inverse_blocked_lp_delay(tw_lpid lp) {
  if (lp >= delay_seed  && lp < delay_seed + g_total_lps*percent_long) {
    return short_delay;
  } else {
    return long_delay;
  }
}

// Maps for load (same as maps for delay)
inline int uniform_lp_load(tw_lpid lp) {
  return light_load;
}

inline int blocked_lp_load(tw_lpid lp) {
  if (lp >= load_seed  && lp < load_seed + g_total_lps*percent_heavy) {
    return heavy_load;
  } else {
    return light_load;
  }
}

inline int linear_lp_load(tw_lpid lp) {
  return ((heavy_load-light_load)*((double)lp / (g_total_lps-1)))+light_load;
}

// Maps for remote (same as maps for delay)
inline tw_stime uniform_lp_remote(tw_lpid lp) {
  return generous_remote;
}

inline tw_stime blocked_lp_remote(tw_lpid lp) {
  if (lp >= remote_seed  && lp < remote_seed + g_total_lps*percent_greedy) {
    return greedy_remote;
  } else {
    return generous_remote;
  }
}

inline tw_stime inverse_blocked_lp_remote(tw_lpid lp) {
  if (lp >= remote_seed  && lp < remote_seed + g_total_lps*percent_greedy) {
    return generous_remote;
  } else {
    return greedy_remote;
  }
}

#endif
