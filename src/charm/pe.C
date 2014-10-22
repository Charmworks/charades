#include "pe.h"

#include "lp.h"
#include "charm_functions.h"

#include "ross_util.h"
#include "ross_api.h"

#include "mpi-interoperate.h"
#include <float.h> // Included for DBL_MAX

CProxy_PE pes;

Globals* get_globals() {
  static PE* local_pe = pes.ckLocalBranch();
  return local_pe->globals;
}

Statistics* get_statistics() {
  static PE* local_pe = pes.ckLocalBranch();
  return local_pe->statistics;
}

// TODO(eric): These should probably be moved to charm_api.C
int tw_ismaster() {
  return (CkMyPe() == 0);
}

int tw_nnodes() {
  return CkNumPes();
}

int tw_mype() {
  return CkMyPe();
}

void tw_abort(const char* error) {
  CkAbort(error);
}

void charm_exit() {
  CharmLibExit();
}

// Starts the simulation by calling the scheduler on all pes
void charm_run() {
  if (tw_ismaster()) {
    DEBUG("[%d] Initializing schedulers \n", CkMyPe());
    PE_VALUE(total_time) = CkWallTimer();
    if(PE_VALUE(g_tw_synchronization_protocol) == SEQUENTIAL) {
      pes.execute_seq();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
      pes.execute_cons();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      pes.execute_opt();
    } else {
      tw_error(TW_LOC, "Incorrect scheduler values, Aborting\n");
    }
  }
  StartCharmScheduler();
}

// Instead of using the general purpose macros we can use optimized ones
#undef PE_VALUE
#define PE_VALUE(x) globals->x

#undef PE_STATS
#define PE_STATS(x) statistics->x

PE::PE(CProxy_Initialize srcProxy) : gvt_cnt(0) {
  // init globals
  // TODO: Maybe make this a function
  // TODO: Make sure all are initialized and make sense
  globals = new Globals;
  globals->g_lps_per_chare = 4;
  globals->g_tw_synchronization_protocol = CONSERVATIVE;
  globals->g_tw_ts_end = 1024;
  globals->g_tw_mblock = 16;
  globals->g_tw_gvt_interval = 16;
  globals->g_tw_nlp = globals->g_lps_per_chare;
  globals->g_tw_rng_seed = NULL;
  globals->g_tw_rng_max = 1;
  globals->g_tw_nRNG_per_lp = 1;
  globals->g_tw_rng_default = 1;
  globals->g_tw_csv = NULL;
  globals->g_tw_max_events_buffered = 1024;
  globals->g_tw_min_detected_offset = DBL_MAX;
  globals->g_tw_lookahead = .005;
  globals->g_init_map = init_block_map;
  globals->g_local_map = local_block_map;
  globals->lastGVT = 0.0;
  globals->netEvents = 0;
  globals->total_time = 0.0;
  gvt = 0.0;

  // init stats
  // TODO: Maybe make this a function
  // TODO: Make sure all are initialized and make sense
  statistics = new Statistics;
  statistics->s_max_run_time = 0.0;
  statistics->s_net_events = 0;
  statistics->s_nevent_processed = 0;
  statistics->s_nevent_abort = 0;
  statistics->s_e_rbs = 0;
  statistics->s_rb_total = 0;
  statistics->s_rb_primary = 0;
  statistics->s_rb_secondary = 0;
  statistics->s_fc_attempts = 0;
  statistics->s_pq_qsize = 0;
  statistics->s_nsend_network = 0;
  statistics->s_nread_network = 0;
  statistics->s_nsend_remote_rb = 0;
  statistics->s_nsend_loc_remote = 0;
  statistics->s_nsend_net_remote = 0;
  statistics->s_ngvts = 0;
  statistics->s_mem_buffers_used = 0;
  statistics->s_pe_event_ties = 0;
  statistics->s_min_detected_offset = 0.0;
  statistics->s_total = 0;
  statistics->s_net_read = 0;
  statistics->s_gvt = 0;
  statistics->s_fossil_collect = 0;
  statistics->s_event_abort = 0;
  statistics->s_event_process = 0;
  statistics->s_pq = 0;
  statistics->s_rollback = 0;
  statistics->s_cancel_q = 0;
  statistics->s_avl = 0;

  cancel_q.resize(0);
  thisProxy[CkMyPe()].initialize_rand(srcProxy);
}

void PE::initialize_rand(CProxy_Initialize srcProxy) {
  rng = tw_rand_init(31, 41);
  contribute(CkCallback(CkReductionTarget(Initialize,Exit),srcProxy));
}

// Pull the next LP from the queue and have it execute events until it hits
// execute_until, or executes max events.
bool PE::schedule_next_lp() {
  LPToken *min = next_lps.top();
  if(min == NULL) return 0;
  if (min->lp->execute_me()) {
    PE_STATS(s_nevent_processed)++;
    return true;
  } else {
    return false;
  }
}

// Compute the minimum time for gvt purposes. We not only need to take into
// account the earliest pending event in the system, but also the earliest
// pending cancellation event.
Time PE::get_min_time() {
  Time min;

  if(next_lps.top() != NULL) {
    min = next_lps.top()->ts;
  } else {
    min = DBL_MAX;
  }

  // TODO: This could probably be optimized
  if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
    for(int pe_i = 0; pe_i < cancel_q.size(); pe_i++) {
      Time new_min = cancel_q[pe_i]->min_cancel_time();
      if(new_min < min) {
        min = new_min;
      }
    }
  }

  return min;
}

// Receives the reduction of the final event count, prints stats, and exits.
void PE::print_final_stats(double total_events) {
  CkPrintf("Total events executed: %.0lf\n", total_events);
  CkPrintf("Total time: %f s\n", PE_VALUE(total_time));
  CkPrintf("Event rate: %f events/s\n", total_events/PE_VALUE(total_time));
  CkExit();
}


/******************************************************************************/
/* Schedulers                                                                 */
/******************************************************************************/

// Just execute events one at a time until the end time.
void PE::execute_seq() {
  while (get_min_time() < PE_VALUE(g_tw_ts_end)) {
    if (!schedule_next_lp()) {
      break;
    }
  }
  CkExit();
}

// Execute events within the current window based on lookahead.
// Because of lookahead constraints we can batch execution by having each LP
// execute all the way up to the next window.
void PE::execute_cons() {
  while (get_min_time() < gvt + PE_VALUE(g_tw_lookahead)) {
    if (!schedule_next_lp()) {
      break;
    }
  }
  gvt_begin();
}

// Execute events speculatively, processing g_tw_mblock messages each iteration.
// Also process the cancellation queue each iteration.
// After a fixed number of iterations, compute a new GVT.
void PE::execute_opt() {
  int events_left = PE_VALUE(g_tw_mblock);
  while (events_left) {
    if(schedule_next_lp()) {
      events_left--;
    } else {
      break;
    }
  }
  process_cancel_q();

  if(++gvt_cnt > PE_VALUE(g_tw_gvt_interval)) {
    gvt_begin();
    gvt_cnt = 0;
  } else {
    thisProxy[CkMyPe()].execute_opt();
  }
}

/******************************************************************************/
/* Methods for optimistic execution only                                      */
/******************************************************************************/

// Call fossil_me on all lps that have fossils older than the current gvt.
// The oldest_lps queue ensures we will only call fossil_me on lps that need it.
void PE::collect_fossils() {
  LPToken *min = oldest_lps.top();
  while((min != NULL) && (min->ts < gvt)) {
    min->lp->fossil_me(gvt);
    min = oldest_lps.top();
  }
}

// Call process_cancel_q on every LP chare in our PE level cancel_q.
void PE::process_cancel_q() {
  for(int pe_i = 0; pe_i < cancel_q.size(); pe_i++) {
    cancel_q[pe_i]->process_cancel_q();
  }
}

/******************************************************************************/
/* GVT methods                                                                */
/******************************************************************************/

// Wait for total quiessence before allowing anyone to contribute to the
// gvt reduction.
void PE::gvt_begin() {
  DEBUG4("******** GVT begins ********\n");
  if(CkMyPe() == 0) {
    /* TODO: Provide option for using completion detection */
    CkStartQD(CkCallback(CkIndex_PE::gvt_contribute(), thisProxy));
  }
}

// Contribute this PEs minimum time to a min reduction to compute the gvt.
void PE::gvt_contribute() {
  Time min_time = get_min_time();
  DEBUG4("******** GVT contribute %lf ********\n", min_time);
  contribute(sizeof(Time), &min_time, CkReduction::min_double,
      CkCallback(CkReductionTarget(PE,gvt_end),thisProxy));
}

// Check to see if we are complete. If not, re-enter the appropriate
// scheduler loop, and possibly do fossil collection.
void PE::gvt_end(Time new_gvt) {
  DEBUG4("******** GVT computed %lf ********\n", new_gvt);
  PE_VALUE(lastGVT) = gvt;
  gvt = new_gvt;
  if(new_gvt >= PE_VALUE(g_tw_ts_end)) {
    PE_VALUE(total_time) = CkWallTimer() - PE_VALUE(total_time);
    contribute(sizeof(double), &(globals->netEvents), CkReduction::sum_double,
        CkCallback(CkReductionTarget(PE,print_final_stats),thisProxy[0]));
  } else {
    if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
      thisProxy[CkMyPe()].execute_cons();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      collect_fossils();
      thisProxy[CkMyPe()].execute_opt();
    }
  }
}

#include "pe.def.h"
