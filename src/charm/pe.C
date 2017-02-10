#include "pe.h"
#include "lp.h"
#include "charm_functions.h"
#include "gvtmanager.h"
#include "scheduler.h"
#include "ross.h"

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

CProxy_PEManager pe_manager_proxy;

Globals* get_globals() {
  static Globals* globals = pe_manager_proxy.ckLocalBranch()->globals;
  return globals;
}

Statistics* get_statistics() {
  static Statistics* statistics = pe_manager_proxy.ckLocalBranch()->current_stats;
  return statistics;
}

CkReduction::reducerType statsReductionType;
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
  pe_manager_proxy.ckLocalBranch()->start_simulation();
  StartCharmScheduler();
}

// Instead of using the general purpose macros we can use optimized ones
#undef PE_VALUE
#define PE_VALUE(x) globals->x

#undef PE_STATS
#define PE_STATS(x) current_stats->x

PEManager::PEManager() {
  DEBUG_PE("PEManager instantiated\n");
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
  thisProxy[CkMyPe()].initialize();
}

void PEManager::start_simulation() {
  start_time = CmiWallTimer();
  scheduler_proxy[CkMyPe()].execute();
}

void PEManager::end_simulation() {
  end_time = CmiWallTimer();
  cumulative_stats->total_time = end_time - start_time;
  contribute(sizeof(Statistics), cumulative_stats, statsReductionType,
      CkCallback(CkIndex_PEManager::finalize(NULL), thisProxy[0]));
}

void PEManager::finalize(CkReductionMsg* msg) {
  Statistics* final_stats = (Statistics*)msg->getData();
  final_stats->print();
  CkExit();
}

/******************************************************************************/
/* Initialization functions                                                   */
/******************************************************************************/

void PEManager::initialize_rand() {
  DEBUG_PE("Random number generator initialized\n");
  rng = tw_rand_init(31, 41);
}

#include "pe.def.h"
