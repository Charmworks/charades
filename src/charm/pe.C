#include "pe.h"

#include "lp.h"
#include "charm_functions.h"

#include "ross_util.h"
#include "ross_api.h"

#include "mpi-interoperate.h"
#include <float.h> // Included for DBL_MAX

CProxy_PE pes;
CkReduction::reducerType statsReductionType;

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
      Time new_min = cancel_q[pe_i]->getMinCancelTime();
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
  print_final_stats(PE_STATS(s_net_events));
  contribute(sizeof(Statistics), pes.ckLocalBranch()->statistics, statsReductionType,
    CkCallback(CkReductionTarget(PE,tw_stats),thisProxy[0]));

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
  PE_STATS(s_fc_attempts)++;
  LPToken *min = oldest_lps.top();
  if ((min != NULL) && (min->ts < gvt)) {
    PE_STATS(s_fossil_collect)++;
  }
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
  PE_STATS(s_ngvts)++;
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
    contribute(sizeof(Statistics), pes.ckLocalBranch()->statistics, statsReductionType,
        CkCallback(CkReductionTarget(PE,tw_stats),thisProxy[0]));
  } else {
    if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
      thisProxy[CkMyPe()].execute_cons();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      collect_fossils();
      thisProxy[CkMyPe()].execute_opt();
    }
  }
}

void PE::tw_stats(CkReductionMsg *m) {

  int  i;
  Statistics *s = (Statistics *)m->getData();
  size_t m_alloc, m_waste;

  // if (0 == g_tw_sim_started)
  //   return;

  // tw_calloc_stats(&m_alloc, &m_waste);

  // This is the target of a group reduction
  // s has been reduced already

  // should never happen
  // if (!tw_ismaster())
  //   return;

#ifndef ROSS_DO_NOT_PRINT
  printf("\n\t: Running Time = %.4f seconds\n", s->s_max_run_time);
  fprintf(PE_VALUE(g_tw_csv), "%.4f,", s->s_max_run_time);

  printf("\nTW Library Statistics:\n");
  show_lld("Total Events Processed", s->s_nevent_processed);
  show_lld("Events Aborted (part of RBs)", s->s_nevent_abort);
  show_lld("Events Rolled Back", s->s_e_rbs);
  show_lld("Event Ties Detected in PE Queues", s->s_pe_event_ties);
        if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE)
            printf("\t%-50s %11.9lf\n",
               "Minimum TS Offset Detected in Conservative Mode",
               (double) s->s_min_detected_offset);
  show_2f("Efficiency", 100.0 * (1.0 - ((double) s->s_e_rbs / (double) s->s_net_events)));
  show_lld("Total Remote (shared mem) Events Processed", s->s_nsend_loc_remote);

  show_2f(
    "Percent Remote Events",
    ( (double)s->s_nsend_loc_remote
    / (double)s->s_net_events)
    * 100.0
  );

  show_lld("Total Remote (network) Events Processed", s->s_nsend_net_remote);
  show_2f(
    "Percent Remote Events",
    ( (double)s->s_nsend_net_remote
    / (double)s->s_net_events)
    * 100.0
  );

  printf("\n");
  show_lld("Total Roll Backs ", s->s_rb_total);
  show_lld("Primary Roll Backs ", s->s_rb_primary);
  show_lld("Secondary Roll Backs ", s->s_rb_secondary);
  show_lld("Fossil Collect Attempts", s->s_fc_attempts);
  show_lld("Total GVT Computations", s->s_ngvts);

  printf("\n");
  show_lld("Net Events Processed", s->s_net_events);
  show_1f(
    "Event Rate (events/sec)",
    ((double)s->s_net_events / s->s_max_run_time)
  );

  printf("\nTW Memory Statistics:\n");
  show_lld("Events Allocated", PE_VALUE(g_tw_max_events_buffered) * PE_VALUE(g_num_lp_chares));
  show_lld("Memory Allocated", m_alloc / 1024);
  show_lld("Memory Wasted", m_waste / 1024);

  if (tw_nnodes() > 1) {
    printf("\n");
    printf("TW Network Statistics:\n");
    show_lld("Remote sends", s->s_nsend_network);
    show_lld("Remote recvs", s->s_nread_network);
  }

/*
  printf("\nTW Data Structure sizes in bytes (sizeof):\n");
  show_lld("PE struct", sizeof(tw_pe));
  show_lld("KP struct", sizeof(tw_kp));
  show_lld("LP struct", sizeof(tw_lp));
  show_lld("LP Model struct", lp->type->state_sz);
  show_lld("LP RNGs", sizeof(*lp->rng));
  show_lld("Total LP", sizeof(tw_lp) + lp->type->state_sz + sizeof(*lp->rng));
  show_lld("Event struct", sizeof(tw_event));
  show_lld("Event struct with Model", sizeof(tw_event) + PE_VALUE(g_tw_msg_sz));
*/

#ifdef ROSS_timing
  printf("\nTW Clock Cycle Statistics (MAX values in secs at %1.4lf GHz):\n", PE_VALUE(g_tw_clock_rate) / 1000000000.0);
  show_4f("Priority Queue (enq/deq)", (double) s->s_pq / PE_VALUE(g_tw_clock_rate));
    show_4f("AVL Tree (insert/delete)", (double) s->s_avl / PE_VALUE(g_tw_clock_rate));
  show_4f("Event Processing", (double) s->s_event_process / PE_VALUE(g_tw_clock_rate));
  show_4f("Event Cancel", (double) s->s_cancel_q / PE_VALUE(g_tw_clock_rate));
  show_4f("Event Abort", (double) s->s_event_abort / PE_VALUE(g_tw_clock_rate));
  printf("\n");
  show_4f("GVT", (double) s->s_gvt / PE_VALUE(g_tw_clock_rate));
  show_4f("Fossil Collect", (double) s->s_fossil_collect / PE_VALUE(g_tw_clock_rate));
  show_4f("Primary Rollbacks", (double) s->s_rollback / PE_VALUE(g_tw_clock_rate));
  show_4f("Network Read", (double) s->s_net_read / PE_VALUE(g_tw_clock_rate));
  show_4f("Total Time (Note: Using Running Time above for Speedup)", (double) s->s_total / PE_VALUE(g_tw_clock_rate));
#endif

  //tw_gvt_stats(stdout);
#endif

  CkExit();
}

void registerStatsReduction(void) {
  statsReductionType = CkReduction::addReducer(statsReduction);
}

CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs) {
  Statistics *s = new Statistics;
  s->s_max_run_time = 0.0;
  s->s_net_events = 0;
  s->s_nevent_processed = 0;
  s->s_nevent_abort = 0;
  s->s_e_rbs = 0;
  s->s_rb_total = 0;
  s->s_rb_primary = 0;
  s->s_rb_secondary = 0;
  s->s_fc_attempts = 0;
  s->s_pq_qsize = 0;
  s->s_nsend_network = 0;
  s->s_nread_network = 0;
  s->s_nsend_remote_rb = 0;
  s->s_nsend_loc_remote = 0;
  s->s_nsend_net_remote = 0;
  s->s_ngvts = 0;
  s->s_mem_buffers_used = 0;
  s->s_pe_event_ties = 0;
  s->s_min_detected_offset = DBL_MAX;
  s->s_total = 0;
  s->s_net_read = 0;
  s->s_gvt = 0;
  s->s_fossil_collect = 0;
  s->s_event_abort = 0;
  s->s_event_process = 0;
  s->s_pq = 0;
  s->s_rollback = 0;
  s->s_cancel_q = 0;
  s->s_avl = 0;

  for (int i = 0; i < nMsg; i++){
    CkAssert(msgs[i]->getSize() == sizeof(Statistics));

    Statistics *c = (Statistics *)msgs[i]->getData();
    s->s_max_run_time = fmax(s->s_max_run_time, c->s_max_run_time);

    s->s_net_events += c->s_net_events;
    s->s_nevent_processed += c->s_nevent_processed;
    s->s_nevent_abort += c->s_nevent_abort;
    s->s_e_rbs += c->s_e_rbs;
    s->s_rb_total += c->s_rb_total;
    s->s_rb_primary += c->s_rb_primary;
    s->s_rb_secondary += c->s_rb_secondary;
    s->s_fc_attempts += c->s_fc_attempts;
    s->s_pq_qsize += c->s_pq_qsize;
    s->s_nsend_network += c->s_nsend_network;
    s->s_nread_network += c->s_nread_network;
    s->s_nsend_remote_rb += c->s_nsend_remote_rb;
    s->s_nsend_loc_remote += c->s_nsend_loc_remote;
    s->s_nsend_net_remote += c->s_nsend_net_remote;
    s->s_ngvts += c->s_ngvts;
    s->s_mem_buffers_used += c->s_mem_buffers_used;

    s->s_pe_event_ties += c->s_pe_event_ties;
    s->s_min_detected_offset = fmin(s->s_min_detected_offset, c->s_min_detected_offset);

    s->s_total = fmax(s->s_total, c->s_total);
    s->s_net_read = fmax(s->s_net_read, c->s_net_read);
    s->s_gvt = fmax(s->s_gvt, c->s_gvt);
    s->s_fossil_collect = fmax(s->s_fossil_collect, c->s_fossil_collect);
    s->s_event_abort = fmax(s->s_event_abort, c->s_event_abort);
    s->s_event_process = fmax(s->s_event_process, c->s_event_process);
    s->s_pq = fmax(s->s_pq, c->s_pq);
    s->s_rollback = fmax(s->s_rollback, c->s_rollback);

    s->s_avl = fmax(s->s_avl, c->s_avl);
  }

  return CkReductionMsg::buildNew(sizeof(Statistics), s);
}

#include "pe.def.h"
