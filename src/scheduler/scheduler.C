#include "scheduler.h"

#include "charm_functions.h"
#include "gvtmanager.h"
#include "lp.h"
#include "ross.h"
#include "globals.h"
#include "ross_random.h"
#include "avl_tree.h"
#include "ross_setup.h" // tmp for AVL_NODE_COUNT
#include "event_buffer.h"

#include <float.h> // Included for DBL_MAX

CProxy_Scheduler scheduler_proxy;

/******************************************************************************/
/* Scheduler Base Class                                                       */
/******************************************************************************/
Scheduler::Scheduler() {
  scheduler_proxy = thisProxy;
  globals = new Globals();
  stats = new Statistics();

  // Initialize rng
  rng = tw_rand_init(31, 41);

  // Initialize event buffers
  PE_VALUE(event_buffer) = new EventBuffer(g_tw_max_events_buffered,
                                           g_tw_max_remote_events_buffered,
                                           g_tw_msg_sz);
  PE_VALUE(abort_event) = PE_VALUE(event_buffer)->get_abort_event();
  DEBUG_PE("Created event buffer with %d events and %d msgs of size %d\n",
      g_tw_max_events_buffered, g_tw_max_remote_events_buffered, g_tw_msg_sz);

  // Contribute to a reduction saying all scheduler instances have been created
  contribute(CkCallback(CkReductionTarget(Scheduler, schedulerReady), thisProxy));
  thisProxy[CkMyPe()].initialize();
}

void Scheduler::start_simulation() {
  start_time = CmiWallTimer();
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

/** Return the minimum LP time on this PE */
// TODO: Keep a "current_time" var per PE and update it when update_next happens
Time Scheduler::get_min_time() const {
  return next_lps.top() != NULL ? next_lps.top()->ts : DBL_MAX;
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

/** Needs to be overridden by each base class to define a scheduler iteration */
void Scheduler::execute() {
  CkAbort("Need to instantiate a specific Scheduler subclass\n");
}

void Scheduler::gvt_resume() {}

void Scheduler::gvt_done(Time gvt) {
  PE_STATS(total_gvts)++;
  if(gvt >= g_tw_ts_end) {
    end_simulation();
  } else {
    thisProxy[CkMyPe()].execute();
  }
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


#include "conservative.h"
#include "optimistic.h"
#include "scheduler.def.h"
