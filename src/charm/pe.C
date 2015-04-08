#include "pe.h"

#include "lp.h"
#include "charm_functions.h"

#include "ross_util.h"
#include "ross_api.h"

#include "mpi-interoperate.h"
#include <float.h> // Included for DBL_MAX

extern CProxy_Initialize mainProxy;
CProxy_PE pes;
extern CProxy_LP lps;
CkReduction::reducerType statsReductionType;
CkReduction::reducerType gvtReductionType;

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

void registerGVTReduction(void) {
  gvtReductionType = CkReduction::addReducer(gvtReduction);
}

CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs) {
  Statistics *s = new Statistics;
  initialize_statistics(s);

  for (int i = 0; i < nMsg; i++) {
    CkAssert(msgs[i]->getSize() == sizeof(Statistics));

    Statistics *c = (Statistics *)msgs[i]->getData();
    add_statistics(s, c);
  }

  return CkReductionMsg::buildNew(sizeof(Statistics), s);
}

CkReductionMsg *gvtReduction(int nMsg, CkReductionMsg **msgs) {
  GVT* new_gvt = new GVT;
  for (int i = 0; i < nMsg; i++) {
    CkAssert(msgs[i]->getSize() == sizeof(GVT));
    GVT* gvt = (GVT*)msgs[i]->getData();
    new_gvt->ts = fmin(new_gvt->ts, gvt->ts);
    new_gvt->type = new_gvt->type | gvt->type;
  }
  return CkReductionMsg::buildNew(sizeof(GVT), new_gvt);
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
      pes.initialize_detectors();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      CkPrintf("**** Starting Parallel Optimistic Simulation ****\n");
      pes.initialize_detectors();
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
    gvt_cnt(0), gvt(0.0), min_sent(DBL_MAX), min_cancel_time(DBL_MAX),
    force_gvt(0), waiting_on_qd(false)  {
  int err = posix_memalign((void **)&globals, 64, sizeof(Globals));
  err = posix_memalign((void**)&statistics, 64, sizeof(Statistics));
  initialize_globals(globals);
  initialize_statistics(statistics);

  cancel_q.resize(0);
  thisProxy[CkMyPe()].initialize_rand();
}

void PE::initialize_rand() {
  DEBUG_PE("Random number generator initialized\n");
  rng = tw_rand_init(31, 41);
  contribute(CkCallback(CkIndex_Initialize::Exit(), mainProxy));
}

void PE::initialize_detectors() {
  max_phase = PE_VALUE(g_tw_gvt_phases);
  if (max_phase > 1 && PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
    CkPrintf("WARNING: Cannot have multiple phases in conservative mode.\n");
    CkPrintf("Setting number of phases to 1\n");
    max_phase = 1;
  }
  if (max_phase == 0) {
    resume_scheduler();
  }
  current_phase = next_phase = 0;
  detector_ready = new bool[max_phase];
  detector_proxies = new CProxy_CompletionDetector[max_phase];
  detector_pointers = new CompletionDetector*[max_phase];
  for (int i = 0; i < max_phase; i++) {
    detector_ready[i] = false;
    detector_pointers[i] = NULL;
    if (CkMyPe() == 0) {
      detector_proxies[i] = CProxy_CompletionDetector::ckNew();
    }
  }
  if (CkMyPe() == 0) {
    thisProxy.broadcast_detector_proxies(max_phase, detector_proxies);
  }
}

void PE::broadcast_detector_proxies(int num, CProxy_CompletionDetector* proxies) {
  for (int i = 0; i < num; i++) {
    detector_proxies[i] = proxies[i];
    detector_pointers[i] = detector_proxies[i].ckLocalBranch();
    if (CkMyPe() == 0) {
      detector_proxies[i].start_detection(CkNumPes(),
          CkCallback(CkIndex_PE::detector_initialized(), thisProxy),
          CkCallback(),
          CkCallback(CkIndex_PE::gvt_contribute(), thisProxy), 0);
    }
  }
}

void PE::detector_initialized() {
  current_phase++;
  if (current_phase == max_phase) {
    detectors_initialized();
  }
}

void PE::detectors_initialized() {
  DEBUG_PE("All %d completion detectors are ready\n", max_phase);
  for (int i = 0; i < max_phase; i++) {
    detector_ready[i] = true;
  }
  current_phase = 0;
  next_phase = (current_phase + 1) % max_phase;
  resume_scheduler();
}

void PE::detector_started() {
  detector_ready[next_phase] = true;
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
    return fmin(next_lps.top()->ts, fmin(min_sent, min_cancel_time));
  } else {
    return fmin(min_sent, min_cancel_time);
  }
}

/******************************************************************************/
/* Schedulers                                                                 */
/******************************************************************************/

// Just execute events one at a time until the end time.
void PE::execute_seq() {
  Time gvt = get_min_time();
  while (gvt < PE_VALUE(g_tw_ts_end)) {
    if (!schedule_next_lp()) {
      break;
    }
    if (gvt / PE_VALUE(g_tw_ts_end) > PE_VALUE(percent_complete)) {
      GVT gvt_struct;
      gvt_struct.ts = gvt;
      gvt_print(&gvt_struct);
    }
    gvt = get_min_time();
  }
  PE_STATS(s_max_run_time) = CkWallTimer() - PE_STATS(s_max_run_time);
  PE_STATS(s_min_run_time) = PE_STATS(s_max_run_time);
  contribute(sizeof(Statistics), statistics, statsReductionType,
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
  if (waiting_on_qd) {
    return;
  }
  unsigned events_executed = 0;
  for (int i = 0; i < PE_VALUE(g_tw_mblock); i++) {
    if (PE_VALUE(event_buffer)->percent_used() <= 0.01) {
      force_gvt = MEM_FORCE;
      break;
    }
    if (!schedule_next_lp()) {
      break;
    }
    events_executed++;
  }
  process_cancel_q();

  // If we weren't able to execute any events, then force a GVT
  if (events_executed == 0 && !force_gvt) {
    if (get_min_time() == DBL_MAX) {
      force_gvt = END_FORCE;
    } else {
      force_gvt = EVENT_FORCE;
    }
  }

  bool ready_for_gvt = ++gvt_cnt > PE_VALUE(g_tw_gvt_interval) || force_gvt;
  if (max_phase) {
    ready_for_gvt = ready_for_gvt && detector_ready[next_phase];
  }

  if (ready_for_gvt) {
    // Right now, broadcasting to start GVT is only supported with QD.
    if (max_phase == 0) {
      thisProxy.gvt_begin();
    } else {
      gvt_begin();
    }
  }
  if (!ready_for_gvt || (max_phase > 1 && !force_gvt)) {
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
  if (waiting_on_qd) {
    return;
  }
  gvt_cnt = 0;
  PE_STATS(s_ngvts)++;
  if (max_phase == 0) {
    DEBUG_PE("GVT #%d: Waiting on QD...\n", PE_STATS(s_ngvts));
    waiting_on_qd = true;
    if(CkMyPe() == 0) {
      // TODO: Can QD be started sooner? Will that improve speed?
      CkStartQD(CkCallback(CkIndex_PE::gvt_contribute(), thisProxy));
    }
  } else {
    min_sent = DBL_MAX;
    detector_pointers[current_phase]->done();
    detector_ready[current_phase] = false;
    current_phase = next_phase;
    next_phase = (current_phase+1)%max_phase;
  }
}

// Contribute this PEs minimum time to a min reduction to compute the gvt.
void PE::gvt_contribute() {
  GVT gvt_struct;
  gvt_struct.ts = get_min_time();
  gvt_struct.type = force_gvt;
  DEBUG_PE("GVT #%d: {%lf, %d}\n", PE_STATS(s_ngvts), gvt_struct.ts, gvt_struct.type);
  if (max_phase == 0) {
    waiting_on_qd = false;
  } else if (CkMyPe() == 0) {
    detector_proxies[next_phase].start_detection(CkNumPes(),
        CkCallback(CkIndex_PE::detector_started(), thisProxy),
        CkCallback(),
        CkCallback(CkIndex_PE::gvt_contribute(), thisProxy), 0);
  }
  contribute(sizeof(GVT), &gvt_struct, gvtReductionType,
      CkCallback(CkReductionTarget(PE,gvt_end),thisProxy));

  // If we are doing optimistic simulation, we don't need to wait for the result
  // of the reduction to continue execution (unless we plan on doing load
  // balancing in this iteration).
  // For now this is only supported with QD.
  bool can_resume = PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC;
  can_resume = can_resume && !force_gvt && max_phase == 0;
  can_resume = can_resume && (!PE_VALUE(g_tw_ldb_interval) || PE_STATS(s_ngvts) % PE_VALUE(g_tw_ldb_interval) != 0);
  if (can_resume) {
    resume_scheduler();
  }
}

// Check to see if we are complete. If not, re-enter the appropriate
// scheduler loop, and possibly do fossil collection.
void PE::gvt_end(CkReductionMsg* msg) {
  DEBUG_PE("Got the reduction\n");
  GVT* gvt_struct = (GVT*)msg->getData();
  Time new_gvt = gvt_struct->ts;

  // Update stats that track forced GVTs
  if (gvt_struct->type) {
    PE_STATS(s_forced_gvts)++;
    if (gvt_struct->type & MEM_FORCE) {
      PE_STATS(s_forced_mem_gvts)++;
    }
    if (gvt_struct->type & END_FORCE) {
      PE_STATS(s_forced_end_gvts)++;
    }
    if (gvt_struct->type & EVENT_FORCE) {
      PE_STATS(s_forced_event_gvts)++;
    }
  }

  // If this GVT is the same as the last, and we are low on event memory, then
  // we will never be able to make progress.
  if (gvt == new_gvt && force_gvt & MEM_FORCE) {
    tw_error(TW_LOC, "[%d]: GVT can't progress: Out of events\n", CkMyPe());
  }

  PE_VALUE(g_last_gvt) = gvt;
  gvt = new_gvt;
  if (tw_ismaster() && gvt/PE_VALUE(g_tw_ts_end) > PE_VALUE(percent_complete)) {
    gvt_print(gvt_struct);
  }

  // Either stop the timer and end the simulation, or call the scheduler again.
  if(new_gvt >= PE_VALUE(g_tw_ts_end)) {
    PE_STATS(s_max_run_time) = CkWallTimer() - PE_STATS(s_max_run_time);
    PE_STATS(s_min_run_time) = PE_STATS(s_max_run_time);
    contribute(sizeof(Statistics), statistics, statsReductionType,
        CkCallback(CkReductionTarget(PE,end_simulation),thisProxy[0]));
  } else {
    if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      DEBUG_PE("Collecting fossils\n");
      collect_fossils();
    }
    // TODO: This doesn't need to be a broadcast
    if (PE_VALUE(g_tw_ldb_interval) &&
        PE_STATS(s_ngvts) % PE_VALUE(g_tw_ldb_interval) == 0) {
      if (tw_ismaster()) {
        lps.load_balance();
      }
    } else if (PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE
        || force_gvt || max_phase == 1) {
      force_gvt = 0;
      resume_scheduler();
    }
  }
}

void PE::gvt_print(GVT* gvt_struct) {
  if (PE_VALUE(gvt_print_interval) == 1.0) {
    return;
  }
  if (PE_VALUE(percent_complete) == 0.0) {
    PE_VALUE(percent_complete) = PE_VALUE(gvt_print_interval);
    return;
  }
  CkPrintf("GVT #%d", PE_STATS(s_ngvts));
  if (gvt_struct->type) {
    CkPrintf(" (FORCED)");
  }
  CkPrintf(": simulation %d%% complete ",
      (int)fmin(100, floor(100 * (gvt_struct->ts/PE_VALUE(g_tw_ts_end)))));
  if (gvt_struct->ts == DBL_MAX) {
    CkPrintf("(GVT = MAX).\n");
  } else {
    CkPrintf("(GVT = %.4f).\n", gvt_struct->ts);
  }
  PE_VALUE(percent_complete) += PE_VALUE(gvt_print_interval);
}

void PE::resume_scheduler() {
  if (PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
    thisProxy[CkMyPe()].execute_cons();
  } else if (PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
    thisProxy[CkMyPe()].execute_opt();
  }
}

// Receives a reduction of statistics for the simulation, prints them, and ends
// the simulation by exiting Charm++.
void PE::end_simulation(CkReductionMsg* m) {
  tw_stats((Statistics*)m->getData());
  CkExit();
}

#include "pe.def.h"
