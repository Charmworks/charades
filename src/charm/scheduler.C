#include "scheduler.h"
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
CProxy_Scheduler scheduler;
extern CProxy_LP lps;
CkReduction::reducerType statsReductionType;

// TODO: Find a better place for all of these non-member functions.
Globals* get_globals() {
  static Globals* globals = scheduler.ckLocalBranch()->globals;
  return globals;
}

Statistics* get_statistics() {
  static Statistics* statistics = scheduler.ckLocalBranch()->current_stats;
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
    scheduler.execute();
  }
  StartCharmScheduler();
}

// Instead of using the general purpose macros we can use optimized ones
#undef PE_VALUE
#define PE_VALUE(x) globals->x

#undef PE_STATS
#define PE_STATS(x) current_stats->x

// TODO: Intialize generics
Scheduler::Scheduler() { initialize(); }
void Scheduler::initialize() {
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

  thisProxy[CkMyPe()].initialize_rand();
  start_time = CmiWallTimer();
}
void Scheduler::initialize_rand() {
  DEBUG_PE("Random number generator initialized\n");
  rng = tw_rand_init(31, 41);
  contribute(CkCallback(CkIndex_Initialize::Exit(), mainProxy));
}

// TODO: Intialize Seq specifics
SequentialScheduler::SequentialScheduler() {}

// TODO: Intialize Cons specifics
ConservativeScheduler::ConservativeScheduler() {}

// TODO: Initialize Opt specifics
OptimisticScheduler::OptimisticScheduler() { cancel_q.resize(0); }

/******************************************************************************/
/* Helper functions                                                           */
/******************************************************************************/

// Pull the next LP from the queue and have it execute events until it hits
// execute_until, or executes max events.
bool Scheduler::schedule_next_lp() {
  LPToken *min = next_lps.top();
  if(min == NULL) return 0;
  if (lps(min->lp->thisIndex).execute_me()) {
    PE_STATS(events_executed)++;
    return true;
  } else {
    return false;
  }
}

// TODO: This varies based on GVT and scheduler type
Time Scheduler::get_min_time() const {
  return next_lps.top() != NULL ? next_lps.top()->ts : DBL_MAX;
}

Time OptimisticScheduler::get_min_time() const {
  if(next_lps.top() != NULL) {
    return fmin(next_lps.top()->ts, min_cancel_time);
  } else {
    return min_cancel_time;
  }
}

// Receives a reduction of statistics for the simulation, prints them, and ends
// the simulation by exiting Charm++.
void Scheduler::end_simulation(CkReductionMsg* m) {
  end_time = CmiWallTimer();
  Statistics* final_stats = (Statistics*)m->getData();
  final_stats->print();
  CkExit();
}

/******************************************************************************/
/* Schedulers                                                                 */
/******************************************************************************/

// Execute events speculatively, processing g_tw_mblock messages each iteration.
// Also process the cancellation queue each iteration.
// After a fixed number of iterations, compute a new GVT.
void Scheduler::execute() {
  CkAbort("Need to instantiate a specific Scheduler subclass\n");
}

void SequentialScheduler::execute() {
  CkAbort("Sequential scheduler not yet implemented\n");
}

void ConservativeScheduler::execute() {
  while (get_min_time() < gvts.ckLocalBranch()->current_gvt() + g_tw_lookahead) {
    if (!schedule_next_lp()) {
      break;
    }
  }
  gvts.ckLocalBranch()->gvt_begin();
}

void OptimisticScheduler::execute() {
  unsigned num_executed;
  for (num_executed = 0; num_executed < g_tw_mblock; num_executed++) {
    if (!schedule_next_lp()) {
      break;
    }
  }
  process_cancel_q();
  iter_cnt++;
  if (iter_cnt > g_tw_gvt_interval) {
    gvts.ckLocalBranch()->gvt_begin();
  } else {
    thisProxy[CkMyPe()].execute();
  }
}

void Scheduler::gvt_resume() {}
void Scheduler::gvt_done(Time gvt) {
  if(gvt >= g_tw_ts_end) {
    end_time = CmiWallTimer();
    cumulative_stats->total_time = end_time - start_time;
    contribute(sizeof(Statistics), cumulative_stats, statsReductionType,
        CkCallback(CkReductionTarget(Scheduler,end_simulation),thisProxy[0]));
  } else {
    thisProxy[CkMyPe()].execute();
  }
}

/******************************************************************************/
/* Methods for optimistic execution only                                      */
/******************************************************************************/

// Call fossil_me on all lps that have fossils older than the current gvt.
// The oldest_lps queue ensures we will only call fossil_me on lps that need it.
// TODO: Take GVT as a param
void OptimisticScheduler::collect_fossils() {
  LPToken *min = oldest_lps.top();
  while((min != NULL) && (min->ts < gvts.ckLocalBranch()->current_gvt())) {
    PE_STATS(fossil_collect_calls)++;
    min->lp->fossil_me(gvts.ckLocalBranch()->current_gvt());
    min = oldest_lps.top();
  }
}

// Call process_cancel_q on every LP chare in our PE level cancel_q.
void OptimisticScheduler::process_cancel_q() {
  vector<LP*> temp_q;
  temp_q.swap(cancel_q);
  min_cancel_time = DBL_MAX;

  for(int i = 0; i < temp_q.size(); i++) {
    temp_q[i]->process_cancel_q();
  }
}

// Add an lp to the cancel queue and check for a new min time.
void OptimisticScheduler::add_to_cancel_q(LP* lp) {
  cancel_q.push_back(lp);
  if (lp->min_cancel_time() < min_cancel_time) {
    min_cancel_time = lp->min_cancel_time();
  }
}

// Check for a new min cancel time.
void OptimisticScheduler::update_min_cancel(Time t) {
  if (t < min_cancel_time) {
    min_cancel_time = t;
  }
}

#include "scheduler.def.h"
