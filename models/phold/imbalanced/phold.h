#ifndef INC_phold_h
#define INC_phold_h

#include "charades.h"

class PHoldMessage {
  public:
    PHoldMessage() : work_load(0), mean_delay(0), percent_remote(0.0) {}
    Time work_load;
    Time mean_delay;
    double percent_remote;
};

class PHoldLP : public LP<PHoldLP, PHoldMessage> {
  private:
    Time work_load;
    Time mean_delay;
    double percent_remote;
  public:
    PHoldLP() {}
    void initialize();
    void forward(PHoldMessage* msg, tw_bf* bf);
    void reverse(PHoldMessage* msg, tw_bf* bf);
    void commit(PHoldMessage* msg, tw_bf* bf);
    void finalize();
};

/*
 * PHOLD Globals
 */
static int start_events = 1;

static double percent_heavy = 0.0;
static int load_map = 0;
static int light_load = 0;
static int heavy_load = 0;
static int load_seed = 0;

static double percent_long = 0.0;
static int delay_map = 0;
static Time short_delay = 1000;
static Time long_delay = 5000;
static int delay_seed = 0;

static double percent_greedy = 0.0;
static int remote_map = 0;
static double generous_remote = 0.1;
static double greedy_remote = 0.0;
static int remote_seed = 0;

static int region_size = 0;

int (*lp_load_map)(uint64_t);
Time (*lp_delay_map)(uint64_t);
double (*lp_remote_map)(uint64_t);

// Maps for delay:
// Uniform is the default, and has every lp use the short delay mean
// Blocked has a contiguous block of lps use the long delay
// Linear has lps using delay proportional to their lpid
inline Time uniform_lp_delay(uint64_t lp) {
  return short_delay;
}

inline Time blocked_lp_delay(uint64_t lp) {
  if (lp >= delay_seed  && lp < delay_seed + g_total_lps*percent_long) {
    return long_delay;
  } else {
    return short_delay;
  }
}

inline Time inverse_blocked_lp_delay(uint64_t lp) {
  if (lp >= delay_seed  && lp < delay_seed + g_total_lps*percent_long) {
    return short_delay;
  } else {
    return long_delay;
  }
}

// Maps for load (same as maps for delay)
inline int uniform_lp_load(uint64_t lp) {
  return light_load;
}

inline int blocked_lp_load(uint64_t lp) {
  if (lp >= load_seed  && lp < load_seed + g_total_lps*percent_heavy) {
    return heavy_load;
  } else {
    return light_load;
  }
}

inline int linear_lp_load(uint64_t lp) {
  return ((heavy_load-light_load)*((double)lp / (g_total_lps-1)))+light_load;
}

// Maps for remote (same as maps for delay)
inline double uniform_lp_remote(uint64_t lp) {
  return generous_remote;
}

inline double blocked_lp_remote(uint64_t lp) {
  if (lp >= remote_seed  && lp < remote_seed + g_total_lps*percent_greedy) {
    return greedy_remote;
  } else {
    return generous_remote;
  }
}

inline double inverse_blocked_lp_remote(uint64_t lp) {
  if (lp >= remote_seed  && lp < remote_seed + g_total_lps*percent_greedy) {
    return generous_remote;
  } else {
    return greedy_remote;
  }
}

#endif
