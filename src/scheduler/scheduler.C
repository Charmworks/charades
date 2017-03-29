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

CkGroupID scheduler_id;

/******************************************************************************/
/* Scheduler Base Class                                                       */
/******************************************************************************/
Scheduler::Scheduler() {
  scheduler_id = thisgroup;
  globals = new Globals();
  stats = new Statistics();
  cumulative_stats = stats;

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
  thisProxy[CkMyPe()].execute();
}

void Scheduler::end_simulation() {
  end_time = CmiWallTimer();
  cumulative_stats->total_time = end_time - start_time;
  contribute(sizeof(Statistics), cumulative_stats, statsReductionType,
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
#if CMK_TRACE_ENABLED
  if (g_tw_stat_interval > 0) {
    cumulative_stats = new Statistics();
    stats->init_tracing();
    stat_trigger.reset(new CountTrigger(g_tw_stat_interval));
  } else {
    stat_trigger.reset(new ConstTrigger(false));
  }
#endif
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
  if (g_tw_ldb_interval > 0) {
    lb_trigger.reset(new CountTrigger(g_tw_ldb_interval));
    if (g_tw_max_ldb > 0) {
      lb_trigger.reset(new BoundedTrigger(lb_trigger.release(), g_tw_max_ldb));
    }
  } else {
    lb_trigger.reset(new ConstTrigger(false));
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
    lb_trigger->iteration_done();
  } else {
    next_iteration();
  }
}

void DistributedScheduler::next_iteration() {
  if (!running && !lb_trigger->ready()) {
    running = true;
    thisProxy[CkMyPe()].execute();
  }
}

void DistributedScheduler::gvt_resume() {}

void DistributedScheduler::gvt_done(Time gvt) {
  PE_STATS(total_gvts)++;
  PE_VALUE(g_last_gvt) = gvt;
#if CMK_TRACE_ENABLED
  stat_trigger->iteration_done();
  // TODO: Should the gvt >= check be done here? or should we just update cumulative in end_sched
  if (stat_trigger->ready() || (g_tw_stat_interval && gvt >= g_tw_ts_end)) {
    cumulative_stats->add(stats);
    stats->log_tracing(cumulative_stats->total_gvts);
    stats->clear();
    stat_trigger->reset();
  }
#endif

  if(gvt >= g_tw_ts_end) {
    end_simulation();
  } else if (lb_trigger->ready()) {
    start_balancing();
  } else {
    next_iteration();
  }
}

void DistributedScheduler::start_balancing() {
  if (running) CkAbort("Can't balance while runnning\n");
  DEBUG_PE("Starting to load balancing\n");
  for (int i = 0; i < next_lps.get_size(); i++) {
    next_lps.as_array()[i]->lp->load_balance();
  }
}

void DistributedScheduler::balancing_complete() {
  DEBUG_PE("Load balancing complete\n");
  lb_trigger->reset();
  next_iteration();
}

#include "conservative.h"
#include "optimistic.h"
#include "scheduler.def.h"
