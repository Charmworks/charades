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

Statistics* get_statistics() {
  static Statistics* statistics = scheduler_proxy.ckLocalBranch()->stats;
  return statistics;
}

CkReduction::reducerType statsReductionType;
void registerStatsReduction(void) {
  statsReductionType = CkReduction::addReducer(statsReduction);
}

CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs) {
  Statistics* s = new Statistics();

  for (int i = 0; i < nMsg; i++) {
    CkAssert(msgs[i]->getSize() == sizeof(Statistics));

    Statistics* c = (Statistics*)msgs[i]->getData();
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
  scheduler_proxy.ckLocalBranch()->start_simulation();
  StartCharmScheduler();
}

// Instead of using the general purpose macros we can use optimized ones
#undef PE_VALUE
#define PE_VALUE(x) globals->x

#undef PE_STATS
#define PE_STATS(x) cumulative_stats->x

#include "pe.def.h"
