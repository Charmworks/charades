#include "pe.h"

#include "lp.h"
#include "charm_functions.h"

#include "ross_util.h"
#include "ross_api.h"

#include "mpi-interoperate.h"
#include <float.h> // Included for DBL_MAX

numlp_map_f g_numlp_map;  // chare -> numlps on that chare
init_map_f  g_init_map;   // (chare x lid) -> gid
type_map_f  g_type_map;   // gid -> type
local_map_f g_local_map;  // gid -> lid
chare_map_f g_chare_map;  // gid -> chare
unsigned g_tw_synchronization_protocol;
tw_stime g_tw_ts_end;       // end time of simulation
unsigned g_tw_mblock;       // number of events per gvt interval
unsigned g_tw_gvt_interval; // number of intervals per gvt
unsigned g_tw_gvt_phases;   // number of phases of the gvt
unsigned g_tw_greedy_start; // whether we allow a greedy start or not
unsigned g_tw_async_reduction; // allow GVT reduction and event exec to overlap
unsigned g_tw_ldb_interval; // number of intervals to wait before ldb
tw_stime g_tw_lookahead;    // event lookahead for conservative
tw_stime g_tw_leash;        // gvt leash for optimistic
double  gvt_print_interval; // determines frequency of progress print outs
size_t    g_tw_rng_max;
unsigned  g_tw_nRNG_per_lp;
unsigned  g_tw_rng_default;
unsigned g_num_chares;    // number of chares
unsigned g_lps_per_chare; // number of LPs per chare (if constant)
unsigned g_total_lps;     // number of LPs in the simulation
size_t        g_tw_msg_sz;
unsigned      g_tw_max_events_buffered;

extern CProxy_Initialize mainProxy;
CProxy_PE pes;
extern CProxy_LP lps;
CkReduction::reducerType statsReductionType;
CkReduction::reducerType gvtReductionType;

// TODO: Find a better place for all of these non-member functions.
Globals* get_globals() {
  static Globals* globals = pes.ckLocalBranch()->globals;
  return globals;
}

Statistics* get_statistics() {
  static Statistics* statistics = pes.ckLocalBranch()->statistics;
  return statistics;
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
  if (tw_ismaster()) {
    DEBUG_MASTER("Initializing schedulers \n");
    if(g_tw_synchronization_protocol == SEQUENTIAL) {
      CkPrintf("**** Starting Sequential Simulation ****\n");
      pes.execute_seq();
    } else if(g_tw_synchronization_protocol == CONSERVATIVE) {
      CkPrintf("**** Starting Parallel Conservative Simulation ****\n");
      pes.initialize_detectors();
    } else if(g_tw_synchronization_protocol == OPTIMISTIC) {
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
    force_gvt(0), waiting_on_gvt(false), gvt_started(false) {

  #ifdef CMK_TRACE_ENABLED
  if (CkMyPe() == 0) {
    traceRegisterUserEvent("Forward Execution", USER_EVENT_FWD);
    traceRegisterUserEvent("Rollback", USER_EVENT_RB);
    traceRegisterUserEvent("Cancellation", USER_EVENT_CANCEL);
    traceRegisterUserEvent("GVT", USER_EVENT_GVT);
    traceRegisterUserEvent("LDB", USER_EVENT_LDB);
  }
  #endif

  int err = posix_memalign((void **)&globals, 64, sizeof(Globals));
  err = posix_memalign((void**)&statistics, 64, sizeof(Statistics));
  initialize_globals(globals);
  initialize_statistics(statistics);

  cancel_q.resize(0);
  thisProxy[CkMyPe()].initialize_rand();
}

/******************************************************************************/
/* Initialization functions                                                   */
/******************************************************************************/

void PE::initialize_rand() {
  DEBUG_PE("Random number generator initialized\n");
  rng = tw_rand_init(31, 41);
  contribute(CkCallback(CkIndex_Initialize::Exit(), mainProxy));
}

void PE::initialize_detectors() {
  max_phase = g_tw_gvt_phases;
  if (max_phase > 1 && g_tw_synchronization_protocol == CONSERVATIVE) {
    CkPrintf("WARNING: Cannot have multiple phases in conservative mode.\n");
    CkPrintf("Setting number of phases to 1\n");
    max_phase = 1;
  }
  if (max_phase == 0) {
    // Start the timer, the scheduler, and QD.
    PE_STATS(s_max_run_time) = CkWallTimer();
    if (CkMyPe() == 0) {
      CkStartQD(CkCallback(CkIndex_PE::gvt_contribute(), thisProxy));
    }
    resume_scheduler();
  } else {
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
}

void PE::broadcast_detector_proxies(int num, CProxy_CompletionDetector* proxies) {
  for (int i = 0; i < num; i++) {
    detector_proxies[i] = proxies[i];
    detector_pointers[i] = detector_proxies[i].ckLocalBranch();
    detector_ready[i] = true;
    if (CkMyPe() == 0) {
      detector_proxies[i].start_detection(CkNumPes(),
          CkCallback(),
          CkCallback(),
          CkCallback(CkIndex_PE::gvt_contribute(), thisProxy), 0);
    }
  }
  current_phase = 0;
  next_phase = (current_phase + 1) % max_phase;

  // Start the timer and the scheduler
  PE_STATS(s_max_run_time) = CkWallTimer();
  contribute(CkCallback(CkIndex_PE::resume_scheduler(), thisProxy));
}

/******************************************************************************/
/* Helper functions                                                           */
/******************************************************************************/

// Pull the next LP from the queue and have it execute events until it hits
// execute_until, or executes max events.
bool PE::schedule_next_lp() {
  LPToken *min = next_lps.top();
  if(min == NULL) return 0;
  if (lps(min->lp->thisIndex).execute_me()) {
  //if (min->lp->execute_me()) {
    PE_STATS(s_nevent_processed)++;
    return true;
  } else {
    return false;
  }
}

// Compute the minimum time for gvt purposes. We not only need to take into
// account the earliest pending event in the system, but also the earliest
// pending cancellation event, and in the case of fully asychronous, the
// minimum event sent out since a phase shift.
Time PE::get_min_time() const {
  if(next_lps.top() != NULL) {
    return fmin(next_lps.top()->ts, fmin(min_sent, min_cancel_time));
  } else {
    return fmin(min_sent, min_cancel_time);
  }
}

// Receives a reduction of statistics for the simulation, prints them, and ends
// the simulation by exiting Charm++.
void PE::end_simulation(CkReductionMsg* m) {
  tw_stats((Statistics*)m->getData());
  CkExit();
}

/******************************************************************************/
/* Schedulers                                                                 */
/******************************************************************************/

bool PE::gvt_ready() const {
  // Use event count instead of leash
  if (g_tw_leash == 0) {
    if (gvt_cnt > g_tw_gvt_interval || force_gvt) {
      if (max_phase == 0 || detector_ready[next_phase]) {
        return true;
      }
    }
  } else {
    if (get_min_time() > gvt + g_tw_leash || force_gvt) {
      if (max_phase == 0 || detector_ready[next_phase]) {
        return true;
      }
    }
  }
  return false;
}

// Just execute events one at a time until the end time.
void PE::execute_seq() {
  PE_STATS(s_max_run_time) = CkWallTimer();
  Time gvt = get_min_time();
  while (gvt < g_tw_ts_end) {
    if (!schedule_next_lp()) {
      break;
    }
    if (gvt / g_tw_ts_end > PE_VALUE(percent_complete)) {
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
  while (get_min_time() < gvt + g_tw_lookahead) {
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
  if (waiting_on_gvt) {
    return;
  }
  unsigned num_executed;
  for (num_executed = 0; num_executed < g_tw_mblock; num_executed++) {
    if (PE_VALUE(event_buffer)->percent_used() <= 0.01) {
      force_gvt = MEM_FORCE;
      break;
    }
    if (!schedule_next_lp()) {
      break;
    }
  }
  process_cancel_q();

  // If we weren't able to execute any events, then force a GVT
  if (num_executed == 0 && !force_gvt) {
    if (get_min_time() == DBL_MAX) {
      force_gvt = END_FORCE;
    } else {
      force_gvt = EVENT_FORCE;
    }
  }

  gvt_cnt++;
  bool ready_for_ldb = g_tw_ldb_interval && (PE_STATS(s_ngvts)+1) % g_tw_ldb_interval == 0;
  if (gvt_ready()) {
    // If greedy_start is allowed, then we can force a GVT early if we've either
    // executed up to the next GVT, or are out of memory (we should not greedy
    // start if we are simply out of events). This is only supported with QD.
    if (g_tw_greedy_start && max_phase <= 1 && force_gvt != END_FORCE
                                            && force_gvt != EVENT_FORCE) {
      thisProxy[0].greedy_gvt_begin();
    } else {
      gvt_begin();
    }
  }
  if (!gvt_ready() || (max_phase > 1 && !force_gvt && !ready_for_ldb)) {
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

// This should only be called on PE 0. If it's the first time, then tell
// all PEs to start the GVT, otherwise do nothing.
void PE::greedy_gvt_begin() {
  if (gvt_started) return;
  gvt_started = true;
  thisProxy.gvt_begin();
}

// Wait for total quiessence before allowing anyone to contribute to the
// gvt reduction.
void PE::gvt_begin() {
  if (waiting_on_gvt) {
    return;
  }
#ifdef CMK_TRACE_ENABLED
  gvt_start = CmiWallTimer();
#endif
  gvt_cnt = 0;
  PE_STATS(s_ngvts)++;
  if (max_phase <= 1) {
    waiting_on_gvt = true;
  }
  if (max_phase) {
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
  if (max_phase <=1) {
    waiting_on_gvt = false;
    gvt_started = false;
  }
  if (max_phase == 0) {
    if (CkMyPe() == 0) {
      CkStartQD(CkCallback(CkIndex_PE::gvt_contribute(), thisProxy));
    }
  } else {
    if (CkMyPe() == 0) {
      detector_proxies[next_phase].start_detection(CkNumPes(),
          CkCallback(),
          CkCallback(),
          CkCallback(CkIndex_PE::gvt_contribute(), thisProxy), 0);
    }
  }
  contribute(sizeof(GVT), &gvt_struct, gvtReductionType,
      CkCallback(CkReductionTarget(PE,gvt_end),thisProxy));

  // If we are doing optimistic simulation, we don't need to wait for the result
  // of the reduction to continue execution (unless we plan on doing load
  // balancing in this iteration).
  if (g_tw_async_reduction && max_phase <= 1) {
    // If we forced the GVT, we should wait until it completes.
    if (!force_gvt) {
      // If we are doing LDB this GVT then we should wait until it completes.
      if (!g_tw_ldb_interval || PE_STATS(s_ngvts % g_tw_ldb_interval != 0)) {
        resume_scheduler();
      }
    }
  }
}

// Check to see if we are complete. If not, re-enter the appropriate
// scheduler loop, and possibly do fossil collection.
void PE::gvt_end(CkReductionMsg* msg) {
  GVT* gvt_struct = (GVT*)msg->getData();
  Time new_gvt = gvt_struct->ts;
  if (max_phase) detector_ready[next_phase] = true;

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
  if (tw_ismaster() && gvt/g_tw_ts_end > PE_VALUE(percent_complete)) {
    gvt_print(gvt_struct);
  }

#ifdef CMK_TRACE_ENABLED
  double gvt_end = CmiWallTimer();
  traceUserBracketEvent(USER_EVENT_GVT, gvt_start, gvt_end);
#endif

  // In multi-phase gvts, we can have multiple that go past end time.
  if (PE_VALUE(g_last_gvt) >= g_tw_ts_end) return;

  // Either stop the timer and end the simulation, or call the scheduler again.
  if(new_gvt >= g_tw_ts_end) {
    PE_STATS(s_max_run_time) = CkWallTimer() - PE_STATS(s_max_run_time);
    PE_STATS(s_min_run_time) = PE_STATS(s_max_run_time);
    contribute(sizeof(Statistics), statistics, statsReductionType,
        CkCallback(CkReductionTarget(PE,end_simulation),thisProxy[0]));
  } else {
    if(g_tw_synchronization_protocol == OPTIMISTIC) {
      BRACKET_TRACE(collect_fossils();, USER_EVENT_FC);
    }
    // TODO: This doesn't need to be a broadcast
    // TODO: Made this a reduction to ensure that all PEs finish fc before lb
    if (g_tw_ldb_interval && PE_STATS(s_ngvts) % g_tw_ldb_interval == 0) {
#ifdef CMK_TRACE_ENABLED
      ldb_start = CmiWallTimer();
#endif
      g_tw_ldb_interval = 0;
      contribute(CkCallback(CkReductionTarget(LP,load_balance), lps));
    } else if (!g_tw_async_reduction || force_gvt) {
      force_gvt = 0;
      if (max_phase <= 1) {
        resume_scheduler();
      }
    }
  }
}

void PE::gvt_print(GVT* gvt_struct) {
  if (gvt_print_interval == 1.0) {
    return;
  }
  if (PE_VALUE(percent_complete) == 0.0) {
    PE_VALUE(percent_complete) = gvt_print_interval;
    return;
  }
  CkPrintf("GVT #%d", PE_STATS(s_ngvts));
  if (gvt_struct->type) {
    CkPrintf(" (FORCED %d)", gvt_struct->type);
  }
  CkPrintf(": simulation %d%% complete ",
      (int)fmin(100, floor(100 * (gvt_struct->ts/g_tw_ts_end))));
  if (gvt_struct->ts == DBL_MAX) {
    CkPrintf("(GVT = MAX).\n");
  } else {
    CkPrintf("(GVT = %.4f).\n", gvt_struct->ts);
  }
  PE_VALUE(percent_complete) += gvt_print_interval;
}

void PE::load_balance_complete() {
#ifdef CMK_TRACE_ENABLED
  double ldb_end = CmiWallTimer();
  traceUserBracketEvent(USER_EVENT_LDB, ldb_start, ldb_end);
#endif
  resume_scheduler();
}

void PE::resume_scheduler() {
  if (g_tw_synchronization_protocol == CONSERVATIVE) {
    thisProxy[CkMyPe()].execute_cons();
  } else if (g_tw_synchronization_protocol == OPTIMISTIC) {
    thisProxy[CkMyPe()].execute_opt();
  }
}

#include "pe.def.h"
