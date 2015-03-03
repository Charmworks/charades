#include "pe.h"

#include "lp.h"
#include "charm_functions.h"

#include "ross_util.h"
#include "ross_api.h"

#include "mpi-interoperate.h"
#include <float.h> // Included for DBL_MAX

CProxy_PE pes;
CkReduction::reducerType statsReductionType;

// TODO: Find a better place for all of these non-member functions.
Globals* get_globals() {
  static PE* local_pe = pes.ckLocalBranch();
  return local_pe->globals;
}

Statistics* get_statistics() {
  static PE* local_pe = pes.ckLocalBranch();
  return local_pe->statistics;
}

void registerStatsReduction(void) {
  statsReductionType = CkReduction::addReducer(statsReduction);
}

CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs) {
  Statistics *s = new Statistics;
  initialize_statistics(s);

  for (int i = 0; i < nMsg; i++) {
    CkAssert(msgs[i]->getSize() == sizeof(Statistics));

    Statistics *c = (Statistics *)msgs[i]->getData();
    // Timing stats
    s->s_max_run_time = fmax(s->s_max_run_time, c->s_max_run_time);
    s->s_min_run_time = fmin(s->s_min_run_time, c->s_min_run_time);

    // Event count stats
    s->s_nevent_processed += c->s_nevent_processed;

    // Rollback stats
    s->s_e_rbs += c->s_e_rbs;
    s->s_rb_total += c->s_rb_total;
    s->s_rb_primary += c->s_rb_primary;
    s->s_rb_secondary += c->s_rb_secondary;

    // Send stats
    s->s_nsend_remote_rb += c->s_nsend_remote_rb;
    s->s_nsend_loc_remote += c->s_nsend_loc_remote;

    // GVT stats
    s->s_ngvts = c->s_ngvts;
    s->s_fc_attempts += c->s_fc_attempts;
    s->s_fossil_collect += c->s_fossil_collect;

    // Currently unused stats // TODO: Document or remove
    s->s_nevent_abort += c->s_nevent_abort;
    s->s_pq_qsize += c->s_pq_qsize;
    s->s_nsend_network += c->s_nsend_network;
    s->s_nread_network += c->s_nread_network;
    s->s_nsend_net_remote += c->s_nsend_net_remote;
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
  PE_STATS(s_max_run_time) = CkWallTimer();
  if (tw_ismaster()) {
    DEBUG_MASTER("Initializing schedulers \n");
    if(PE_VALUE(g_tw_synchronization_protocol) == SEQUENTIAL) {
      CkPrintf("**** Starting Sequential Simulation ****\n");
      pes.execute_seq();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
      CkPrintf("**** Starting Parallel Conservative Simulation ****\n");
      pes.execute_cons();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      CkPrintf("**** Starting Parallel Optimistic Simulation ****\n");
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

PE::PE(CProxy_Initialize srcProxy) :
    gvt_cnt(0), gvt(0.0), min_cancel_time(DBL_MAX)  {
    int err = posix_memalign((void **)&globals, 64, sizeof(Globals));
  initialize_globals(globals);

  statistics = new Statistics;
  initialize_statistics(statistics);

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
  if(next_lps.top() != NULL) {
    return std::min(next_lps.top()->ts, min_cancel_time);
  } else {
    return min_cancel_time;
  }
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
  PE_STATS(s_max_run_time) = CkWallTimer() - PE_STATS(s_max_run_time);
  PE_STATS(s_min_run_time) = PE_STATS(s_max_run_time);
  contribute(sizeof(Statistics), pes.ckLocalBranch()->statistics, statsReductionType,
    CkCallback(CkReductionTarget(PE,end_simulation),thisProxy[0]));

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
  vector<LP*> temp_q;
  temp_q.swap(cancel_q);
  min_cancel_time = DBL_MAX;

  for(int i = 0; i < temp_q.size(); i++) {
    temp_q[i]->process_cancel_q();
  }
}

// Add an lp to the cancel queue and check for a new min time.
void PE::add_to_cancel_q(LP* lp) {
  cancel_q.push_back(lp);
  if (lp->min_cancel_time() < min_cancel_time) {
    min_cancel_time = lp->min_cancel_time();
  }
}

// Check for a new min cancel time.
void PE::update_min_cancel(Time t) {
  if (t < min_cancel_time) {
    min_cancel_time = t;
  }
}

/******************************************************************************/
/* GVT methods                                                                */
/******************************************************************************/

// Wait for total quiessence before allowing anyone to contribute to the
// gvt reduction.
void PE::gvt_begin() {
  PE_STATS(s_ngvts)++;
  DEBUG_PE("GVT #%d: begins\n", PE_STATS(s_ngvts));
  if(CkMyPe() == 0) {
    /* TODO: Provide option for using completion detection */
    CkStartQD(CkCallback(CkIndex_PE::gvt_contribute(), thisProxy));
  }
}

// Contribute this PEs minimum time to a min reduction to compute the gvt.
void PE::gvt_contribute() {
  Time min_time = get_min_time();
  DEBUG_PE("GVT #%d: contributed %lf\n", PE_STATS(s_ngvts), min_time);
  contribute(sizeof(Time), &min_time, CkReduction::min_double,
      CkCallback(CkReductionTarget(PE,gvt_end),thisProxy));
}

// Check to see if we are complete. If not, re-enter the appropriate
// scheduler loop, and possibly do fossil collection.
void PE::gvt_end(Time new_gvt) {
  PE_VALUE(g_last_gvt) = gvt;
  gvt = new_gvt;
  DEBUG_MASTER("GVT #%d: simulation %d%% complete (GVT = %.4f).\n",
      PE_STATS(s_ngvts),
      (int) fmin(100, floor(100 *(gvt/PE_VALUE(g_tw_ts_end)))),
      gvt);

  // Either stop the timer and end the simulation, or call the scheduler again.
  if(new_gvt >= PE_VALUE(g_tw_ts_end)) {
    PE_STATS(s_max_run_time) = CkWallTimer() - PE_STATS(s_max_run_time);
    PE_STATS(s_min_run_time) = PE_STATS(s_max_run_time);
    contribute(sizeof(Statistics), pes.ckLocalBranch()->statistics, statsReductionType,
        CkCallback(CkReductionTarget(PE,end_simulation),thisProxy[0]));
  } else {
    if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
      thisProxy[CkMyPe()].execute_cons();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      collect_fossils();
      thisProxy[CkMyPe()].execute_opt();
    }
  }
}

// Receives a reduction of statistics for the simulation, prints them, and ends
// the simulation by exiting Charm++.
void PE::end_simulation(CkReductionMsg* m) {
  tw_stats((Statistics*)m->getData());
  CkExit();
}

#include "pe.def.h"
