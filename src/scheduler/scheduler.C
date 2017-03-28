#include "scheduler.h"

#include "charm_functions.h"
#include "gvtmanager.h"
#include "lp.h"
#include "ross.h"
#include "globals.h"
#include "ross_random.h"
#include "event_buffer.h"
#include "trigger.h"

#include <float.h> // Included for DBL_MAX

CProxy_Scheduler scheduler_proxy;

/******************************************************************************/
/* Scheduler Base Class                                                       */
/******************************************************************************/
Scheduler::Scheduler() {
  scheduler_proxy = thisProxy;
  globals = new Globals();
  stats = new Statistics();

  running = false;

  // Initialize rng
  rng = tw_rand_init(31, 41);

  // Initialize event buffers
  PE_VALUE(event_buffer) = new EventBuffer(g_tw_max_events_buffered,
                                           g_tw_max_remote_events_buffered,
                                           g_tw_msg_sz);
  PE_VALUE(abort_event) = PE_VALUE(event_buffer)->get_abort_event();
  DEBUG_PE("Created event buffer with %d events and %d msgs of size %d\n",
      g_tw_max_events_buffered, g_tw_max_remote_events_buffered, g_tw_msg_sz);

  if (CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_Scheduler::groups_created(), thisProxy));
  }
}

void Scheduler::groups_created() {
  // After groups are created, create LP array, and exit upon quiescence
  if (CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_Initialize::Exit(), mainProxy));
    CProxy_LP::ckNew(g_num_chares);
  }
}

void Scheduler::start_simulation() {
  start_time = CmiWallTimer();
  running = true;
  scheduler_proxy[CkMyPe()].execute();
}

void Scheduler::end_simulation() {
  end_time = CmiWallTimer();
  stats->total_time = end_time - start_time;
  contribute(sizeof(Statistics), stats, statsReductionType,
      CkCallback(CkIndex_Scheduler::finalize(NULL), thisProxy[0]));
}

void Scheduler::finalize(CkReductionMsg* msg) {
  Statistics* final_stats = (Statistics*)msg->getData();
  final_stats->print();
  CkExit();
}

/** Pull the next LP from the queue and have it execute an event */
bool Scheduler::schedule_next_lp() {
  LPToken *min = next_lps.top();
  if(min == NULL) return false;
  if (lps(min->lp->thisIndex).execute_me()) {
    PE_STATS(events_executed)++;
    return true;
  } else {
    return false;
  }
}

/** Return the minimum LP time on this PE */
// TODO: Keep a "current_time" var per PE and update it when update_next happens
Time Scheduler::get_min_time() const {
  return next_lps.top() != NULL ? next_lps.top()->ts : DBL_MAX;
}

/** Needs to be overridden by each base class to define a scheduler iteration */
void Scheduler::execute() {
  CkAbort("Must instantiate a particular scheduler type!\n");
}

/******************************************************************************/
/* Sequential Scheduler                                                       */
/******************************************************************************/
SequentialScheduler::SequentialScheduler() {
  scheduler_name = "Sequential Scheduler";
}

void SequentialScheduler::execute() {
  while (schedule_next_lp());
  end_simulation();
}

/******************************************************************************/
/* Distributed Scheduler                                                      */
/******************************************************************************/
DistributedScheduler::DistributedScheduler() {
  if (CkMyPe() == 0) {
    switch (g_tw_gvt_scheme) {
      case 1:
        CProxy_SyncGVT::ckNew();
        break;
      case 2:
        CProxy_PhaseGVT::ckNew();
        break;
      default:
        CkAbort("Unknown gvt scheme\n");
    }
  }
}

void DistributedScheduler::groups_created() {
  // Grab local pointers to GVT manager, and inform the manager that groups are
  // created, then do call the super class method.
  gvt_manager = gvt_manager_proxy.ckLocalBranch();
  gvt_manager->groups_created();
  Scheduler::groups_created();
}

void DistributedScheduler::iteration_done() {
  running = false;
  gvt_trigger->iteration_done();
  if (gvt_trigger->ready()) {
    gvt_manager->gvt_begin();
    gvt_trigger->reset();
  } else {
    next_iteration();
  }
}

void DistributedScheduler::next_iteration() {
  if (!running) {
    running = true;
    thisProxy[CkMyPe()].execute();
  }
}

void DistributedScheduler::gvt_resume() {}

void DistributedScheduler::gvt_done(Time gvt) {
  PE_STATS(total_gvts)++;
  if(gvt >= g_tw_ts_end) {
    end_simulation();
  } else {
    next_iteration();
  }
}

#include "conservative.h"
#include "optimistic.h"
#include "scheduler.def.h"
