#include "pe.h"
#include "lp.h"
#include "charm_functions.h"
#include "gvtsynch.h"

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
unsigned g_tw_expected_events;
tw_stime g_tw_ts_end;       // end time of simulation
unsigned g_tw_mblock;       // number of events per gvt interval
unsigned g_tw_gvt_interval; // number of intervals per gvt
unsigned g_tw_gvt_phases;   // number of phases of the gvt
unsigned g_tw_greedy_start; // whether we allow a greedy start or not
unsigned g_tw_async_reduction; // allow GVT reduction and event exec to overlap
unsigned g_tw_ldb_interval; // number of intervals to wait before ldb
unsigned g_tw_max_ldb;      // max number of times we will load balance
unsigned g_tw_stat_interval;// number of intervals between stat logging
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
unsigned      g_tw_max_remote_events_buffered;

extern CProxy_Initialize mainProxy;
CProxy_PE pes;
extern CProxy_GvtSynch gvts;
extern CProxy_LP lps;
CkReduction::reducerType statsReductionType;



// TODO: Find a better place for all of these non-member functions.
Globals* get_globals() {
  static Globals* globals = pes.ckLocalBranch()->globals;
  return globals;
}

Statistics* get_statistics() {
  static Statistics* statistics = pes.ckLocalBranch()->current_stats;
  return statistics;
}

void registerStatsReduction(void) {
  statsReductionType = CkReduction::addReducer(statsReduction);
}



CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs) {
  Statistics *s = new Statistics();

  for (int i = 0; i < nMsg; i++) {
    CkAssert(msgs[i]->getSize() == sizeof(Statistics));

    Statistics *c = (Statistics *)msgs[i]->getData();
    s->reduce(c);
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
#define PE_STATS(x) current_stats->x

PE::PE(CProxy_Initialize srcProxy) :
    gvt_num(0), iter_cnt(0), gvt(0.0), leash_start(0.0), min_sent(DBL_MAX),
    min_cancel_time(DBL_MAX), force_gvt(0), waiting_on_gvt(false),
    gvt_started(false), ldb_cnt(0) {

  int err = posix_memalign((void **)&globals, 64, sizeof(Globals));
  err = posix_memalign((void**)&current_stats, 64, sizeof(Statistics));
  err = posix_memalign((void**)&cumulative_stats, 64, sizeof(Statistics));
  clear_globals(globals);
  current_stats->clear();
  cumulative_stats->clear();

#ifdef CMK_TRACE_ENABLED
  if (CkMyPe() == 0) {
    traceRegisterUserEvent("Forward Execution", USER_EVENT_FWD);
    traceRegisterUserEvent("Rollback", USER_EVENT_RB);
    traceRegisterUserEvent("Cancellation", USER_EVENT_CANCEL);
    traceRegisterUserEvent("GVT", USER_EVENT_GVT);
    traceRegisterUserEvent("LDB", USER_EVENT_LDB);
  }
  current_stats->init_tracing();
#endif

  cancel_q.resize(0);
  thisProxy[CkMyPe()].initialize_rand();
  // TODO: tighter start and end time
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
    //if (CkMyPe() == 0) {
    //  CkStartQD(CkCallback(CkIndex_PE::gvt_contribute(), thisProxy));
    //}
    start_time = CmiWallTimer();
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
      //thisProxy.broadcast_detector_proxies(max_phase, detector_proxies);
    }
  }
}

void PE::broadcast_detector_proxies(int num, CProxy_CompletionDetector* proxies) {
  /*for (int i = 0; i < num; i++) {
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
  start_time = CmiWallTimer();
  contribute(CkCallback(CkIndex_PE::resume_scheduler(), thisProxy));
*/
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
    PE_STATS(events_executed)++;
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
  end_time = CmiWallTimer();
  Statistics* final_stats = (Statistics*)m->getData();
  final_stats->print();
  CkExit();
}

/******************************************************************************/
/* Schedulers                                                                 */
/******************************************************************************/

bool PE::gvt_ready() const {
  // Use event count instead of leash
  if (g_tw_leash == 0) {
    if (iter_cnt > g_tw_gvt_interval || force_gvt) {
      if (max_phase == 0 || detector_ready[next_phase]) {
        return true;
      }
    }
  } else {
    LPToken* min = next_lps.top();
    if (min == NULL) return true;
    if (min->ts > leash_start + g_tw_leash || force_gvt) {
      if (max_phase == 0 || detector_ready[next_phase]) {
        return true;
      }
    }
  }
  return false;
}

// Just execute events one at a time until the end time.
void PE::execute_seq() {
  start_time = CmiWallTimer();
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
  end_time = CmiWallTimer();
  cumulative_stats->add(current_stats);
  cumulative_stats->total_time = end_time - start_time;
  contribute(sizeof(Statistics), cumulative_stats, statsReductionType,
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
  gvt_num++;
  //gvt_begin();
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

  iter_cnt++;
  bool ready_for_ldb = g_tw_ldb_interval && gvt_num+1 % g_tw_ldb_interval == 0;
  bool do_gvt = gvt_ready();
  if (do_gvt) {
    // If greedy_start is allowed, then we can force a GVT early if we've either
    // executed up to the next GVT, or are out of memory (we should not greedy
    // start if we are simply out of events). This is only supported with QD.
    if (g_tw_greedy_start && max_phase <= 1 && force_gvt != END_FORCE
                                            && force_gvt != EVENT_FORCE) {
      //thisProxy[0].greedy_gvt_begin();
    } else {
     //CALL PROPER GVT METHOD 
      gvt_num++;
      iter_cnt = 0;
      gvts.ckLocalBranch()->gvt_begin();
    }
  }
  if (!do_gvt || (max_phase > 1 && !force_gvt && !ready_for_ldb)) {
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
    PE_STATS(fossil_collect_calls)++;
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

/*
void PE::greedy_gvt_begin() {
  if (gvt_started) return;
  gvt_started = true;
  thisProxy.gvt_begin();
} */

// Wait for total quiessence before allowing anyone to contribute to the
// gvt reduction.


// Check to see if we are complete. If not, re-enter the appropriate
// scheduler loop, and possibly do fossil collection.
void PE::gvt_done(GVT * gvt_struct) {

  Time new_gvt = gvt_struct->ts;

  // Update stats that track forced GVTs
  PE_STATS(total_gvts)++;
  if (gvt_struct->type) {
    PE_STATS(total_forced_gvts)++;
    if (gvt_struct->type & MEM_FORCE) {
      PE_STATS(mem_forced_gvts)++;
    }
    if (gvt_struct->type & END_FORCE) {
      PE_STATS(end_forced_gvts)++;
    }
    if (gvt_struct->type & EVENT_FORCE) {
      PE_STATS(event_forced_gvts)++;
    }
  }
  // If this GVT is the same as the last, and we are low on event memory, then
  // we will never be able to make progress.
  if (gvt == new_gvt && force_gvt & MEM_FORCE) {
    tw_error(TW_LOC, "[%d]: GVT can't progress: Out of events\n", CkMyPe());
  }

  PE_VALUE(g_last_gvt) = gvt;
  gvt = new_gvt;
  leash_start = new_gvt;
  if (tw_ismaster() && gvt/g_tw_ts_end > PE_VALUE(percent_complete)) {
    gvt_print(gvt_struct);
  }

#ifdef CMK_TRACE_ENABLED
  double gvt_end = CmiWallTimer();
  traceUserBracketEvent(USER_EVENT_GVT, gvt_start, gvt_end);
#endif

  // In multi-phase gvts, we can have multiple that go past end time.
  if (PE_VALUE(g_last_gvt) >= g_tw_ts_end) return;

  if(g_tw_synchronization_protocol == OPTIMISTIC) {
    BRACKET_TRACE(collect_fossils();, USER_EVENT_FC);
  }

  PE_STATS(max_events_used) = PE_VALUE(event_buffer)->memory_stats.max_allocated;
  PE_STATS(new_event_calls) = PE_VALUE(event_buffer)->memory_stats.remote_new_allocated - cumulative_stats->new_event_calls;
  PE_STATS(del_event_calls) = PE_VALUE(event_buffer)->memory_stats.remote_deallocated - cumulative_stats->del_event_calls;
  //log_stats();

  if (gvt_num % g_tw_stat_interval == 0 || new_gvt >= g_tw_ts_end) {
#ifdef CMK_TRACE_ENABLED
    current_stats->log_tracing(gvt_num);
#endif
    cumulative_stats->add(current_stats);
    current_stats->clear();
  }

  // Either stop the timer and end the simulation, or call the scheduler again.
  if(new_gvt >= g_tw_ts_end) {
    end_time = CmiWallTimer();
    cumulative_stats->total_time = end_time - start_time;
    contribute(sizeof(Statistics), cumulative_stats, statsReductionType,
        CkCallback(CkReductionTarget(PE,end_simulation),thisProxy[0]));
  } else {
    // TODO: This doesn't need to be a broadcast
    if (g_tw_ldb_interval && gvt_num % g_tw_ldb_interval == 0) {
#ifdef CMK_TRACE_ENABLED
      ldb_start = CmiWallTimer();
#endif
      if (ldb_cnt++ >= g_tw_max_ldb) {
        g_tw_ldb_interval = 0;
      }
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
  CkPrintf("GVT #%d", gvt_num);
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

unsigned PE::get_gvt_type() {
  return force_gvt;
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
