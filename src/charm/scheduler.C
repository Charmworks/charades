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
CkReduction::reducerType gvtReductionType;

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

void registerGVTReduction(void) {
  gvtReductionType = CkReduction::addReducer(gvtReduction);
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
    CkPrintf("Starting scheduler\n");
    scheduler.execute();
  }
  StartCharmScheduler();
}

// Instead of using the general purpose macros we can use optimized ones
#undef PE_VALUE
#define PE_VALUE(x) globals->x

#undef PE_STATS
#define PE_STATS(x) current_stats->x

Scheduler::Scheduler() :
    gvt_num(0), iter_cnt(0), gvt(0.0), min_cancel_time(DBL_MAX), ldb_cnt(0) {}

Scheduler::Scheduler(CProxy_Initialize srcProxy) :
    gvt_num(0), iter_cnt(0), gvt(0.0), min_cancel_time(DBL_MAX), ldb_cnt(0) {

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
  start_time = CmiWallTimer();
}

ConservativeScheduler::ConservativeScheduler(CProxy_Initialize srcProxy) {
//    gvt_num(0), iter_cnt(0), gvt(0.0), min_cancel_time(DBL_MAX), ldb_cnt(0) {

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
  start_time = CmiWallTimer();
}

/******************************************************************************/
/* Initialization functions                                                   */
/******************************************************************************/

void Scheduler::initialize_rand() {
  DEBUG_PE("Random number generator initialized\n");
  rng = tw_rand_init(31, 41);
  contribute(CkCallback(CkIndex_Initialize::Exit(), mainProxy));
}

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

// Compute the minimum time for gvt purposes. We not only need to take into
// account the earliest pending event in the system, but also the earliest
// pending cancellation event, and in the case of fully asychronous, the
// minimum event sent out since a phase shift.
Time Scheduler::get_min_time() const {
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
  CkPrintf("Doing an interval\n");
  unsigned num_executed;
  for (num_executed = 0; num_executed < g_tw_mblock; num_executed++) {
    if (!schedule_next_lp()) {
      break;
    }
  }
  process_cancel_q();
  iter_cnt++;
  if (iter_cnt > g_tw_gvt_interval) {
    gvt_begin();
  } else {
    thisProxy[CkMyPe()].execute();
  }
}

void ConservativeScheduler::execute() {
  while (get_min_time() < gvt + g_tw_lookahead) {
    if (!schedule_next_lp()) {
      break;
    }
  }
  gvt_begin();
}

/******************************************************************************/
/* Methods for optimistic execution only                                      */
/******************************************************************************/

// Call fossil_me on all lps that have fossils older than the current gvt.
// The oldest_lps queue ensures we will only call fossil_me on lps that need it.
void Scheduler::collect_fossils() {
  LPToken *min = oldest_lps.top();
  while((min != NULL) && (min->ts < gvt)) {
    PE_STATS(fossil_collect_calls)++;
    min->lp->fossil_me(gvt);
    min = oldest_lps.top();
  }
}

// Call process_cancel_q on every LP chare in our PE level cancel_q.
void Scheduler::process_cancel_q() {
  vector<LP*> temp_q;
  temp_q.swap(cancel_q);
  min_cancel_time = DBL_MAX;

  for(int i = 0; i < temp_q.size(); i++) {
    temp_q[i]->process_cancel_q();
  }
}

// Add an lp to the cancel queue and check for a new min time.
void Scheduler::add_to_cancel_q(LP* lp) {
  cancel_q.push_back(lp);
  if (lp->min_cancel_time() < min_cancel_time) {
    min_cancel_time = lp->min_cancel_time();
  }
}

// Check for a new min cancel time.
void Scheduler::update_min_cancel(Time t) {
  if (t < min_cancel_time) {
    min_cancel_time = t;
  }
}

/******************************************************************************/
/* GVT methods                                                                */
/******************************************************************************/

// Wait for total quiessence before allowing anyone to contribute to the
// gvt reduction.
void Scheduler::gvt_begin() {
#ifdef CMK_TRACE_ENABLED
  gvt_start = CmiWallTimer();
#endif
  iter_cnt = 0;
  gvt_num++;
  if (CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_Scheduler::gvt_contribute(), thisProxy));
  }
}

// Contribute this PEs minimum time to a min reduction to compute the gvt.
void Scheduler::gvt_contribute() {
  GVT gvt_struct;
  gvt_struct.ts = get_min_time();
  contribute(sizeof(GVT), &gvt_struct, gvtReductionType,
      CkCallback(CkReductionTarget(Scheduler,gvt_end),thisProxy));
}

// Check to see if we are complete. If not, re-enter the appropriate
// scheduler loop, and possibly do fossil collection.
void Scheduler::gvt_end(CkReductionMsg* msg) {
  GVT* gvt_struct = (GVT*)msg->getData();
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
  if (gvt == new_gvt) {
    tw_error(TW_LOC, "[%d]: GVT can't progress: Out of events\n", CkMyPe());
  }

  PE_VALUE(g_last_gvt) = gvt;
  gvt = new_gvt;
  if (tw_ismaster() && gvt/g_tw_ts_end > PE_VALUE(percent_complete)) {
    //gvt_print(gvt_struct);
  }

#ifdef CMK_TRACE_ENABLED
  double gvt_end = CmiWallTimer();
  traceUserBracketEvent(USER_EVENT_GVT, gvt_start, gvt_end);
#endif

  // In multi-phase gvts, we can have multiple that go past end time.
  if (PE_VALUE(g_last_gvt) >= g_tw_ts_end) return;

  BRACKET_TRACE(collect_fossils();, USER_EVENT_FC);

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
        CkCallback(CkReductionTarget(Scheduler,end_simulation),thisProxy[0]));
  } else {
    thisProxy[CkMyPe()].execute();
  }
}

/*void Scheduler::gvt_print(GVT* gvt_struct) {
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
}*/

#include "scheduler.def.h"
