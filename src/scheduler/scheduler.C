#include "scheduler.h"

#include "charm_setup.h"
#include "event_buffer.h"
#include "globals.h"
#include "gvtmanager.h"
#include "lp.h"
#include "ross_random.h"
#include "statistics.h"
#include "trigger.h"
#include "util.h"

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

  // Initialize event buffer
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
  DEBUG_PE("%s created!\n", scheduler_name.c_str());
  // After groups are created, create LP array, and exit upon quiescence
  if (CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_Initialize::Exit(), mainProxy));
    CProxy_LP::ckNew(g_num_chares);
  }
}

void Scheduler::start_simulation() {
  if (CkMyPe() == 0) {
    CkPrintf("Simulation Starting: %.2f MB\n", CmiMemoryUsage()/(1024.0*1024.0));
  }
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
  if (g_tw_expected_events >= 0 && g_tw_expected_events != final_stats->events_committed) {
    CkPrintf("Expected %i events to be committed, but instead got %i\n",
        g_tw_expected_events, final_stats->events_committed);
    CkAbort("Failed Simulation Expectations!\n");
  } else {
    CkPrintf("Simulation Executed Successfully!\n");
    CkExit();
  }
}

void Scheduler::print_progress(Time ts) {
  if (CkMyPe() != 0) return;
  double time = fmin(ts, g_tw_ts_end);
  double percent = fmin(100*ts/g_tw_ts_end,100.00);
  double mb = CmiMemoryUsage()/(1024.0*1024.0);
  int gvt_num = cumulative_stats->total_gvts;
  if (cumulative_stats != stats) gvt_num += stats->total_gvts;
  CkPrintf("GVT %6i: %8.2f (%6.2f%%), %.2f MB\n", gvt_num, time, percent, mb);
}

/** Attempt to execute the next LPs next event, returning false if it fails */
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

// TODO: Could this be stored in a member variable and updated consistenly?
/** Return the time of the minimum unexecuted event on this PE */
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
  // Create the correct GVT Manager, which will be created before QD triggers
  if (CkMyPe() == 0) {
    switch (g_tw_gvt_scheme) {
      case 1:
        CProxy_SyncGVT::ckNew();
        break;
      case 2:
        CProxy_CdGVT::ckNew();
        break;
      case 3:
        CProxy_PhaseGVT::ckNew();
        break;
      case 4:
        CProxy_BucketGVT::ckNew();
        break;

      default:
        CkAbort("Unknown gvt scheme\n");
    }
  }

  // Set up the load balancing trigger
  if (g_tw_ldb_interval > 0) {
    lb_trigger.reset(new CountTrigger(g_tw_ldb_interval));
    if (g_tw_max_ldb > 0) {
      lb_trigger.reset(new BoundedTrigger(lb_trigger.release(), g_tw_max_ldb));
    }
  } else {
    lb_trigger.reset(new ConstTrigger(false));
  }

  // Set up the print trigger
  print_trigger.reset(new CountTrigger(gvt_print_interval));

  // Set up the statistics logging trigger
#if CMK_TRACE_ENABLED
  if (g_tw_stat_interval > 0) {
    cumulative_stats = new Statistics();
    stats->init_tracing();
    stat_trigger.reset(new CountTrigger(g_tw_stat_interval));
  } else {
    stat_trigger.reset(new ConstTrigger(false));
  }
#endif
}

/** Called by QD, which now also includes GVT Manager creation */
void DistributedScheduler::groups_created() {
  gvt_manager = gvt_manager_proxy.ckLocalBranch();
  gvt_manager->groups_created();
  Scheduler::groups_created();
}

/**
 * Helper method that should be called at the end of every scheduler iteration.
 * It checks the GVT trigger to see if we need to compute a GVT, and continues
 * the scheduler otherwise.
 */
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

/**
 * Helper method for starting the next scheduler iteration. It ensures that an
 * iteration will not start if an iteration is already running or if we are
 * going to do load balancing this iteration after the current iteration.
 */
void DistributedScheduler::next_iteration() {
  if (!running && !lb_trigger->ready()) {
    running = true;
    thisProxy[CkMyPe()].execute();
  }
}

/** By default, don't allow LP execution to overlap with GVT computation */
void DistributedScheduler::gvt_resume() {}

/**
 * After a GVT completes there are a number of things to check:
 * The stat_trigger determines if we should log stats
 * The lb_trigger determines if we should load balance before the next iteration
 * The computed GVT determines if we are past the end time and the sim is over
 * If none of the above are true, then just start the next iteration
 */
void DistributedScheduler::gvt_done(Time gvt) {
  TW_ASSERT(gvt >= PE_VALUE(g_last_gvt), "GVT Causality Violation\n");
  PE_STATS(total_gvts)++;
  PE_VALUE(g_last_gvt) = gvt;

  // TODO: Make stats trigger work like print trigger does below.
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

  print_trigger->iteration_done();
  if(gvt >= g_tw_ts_end) {
    print_progress(gvt);
    end_simulation();
  } else if (lb_trigger->ready()) {
    print_progress(gvt);
    start_balancing();
  } else {
    if (print_trigger->ready()) {
      print_progress(gvt);
    }
    next_iteration();
  }

  // If printing is ready, it would have been handled in one of the if branches
  // so just check to reset the trigger. This can't happen in the else branch
  // for cases where lb happens in the same iteration as the trigger is ready.
  if (print_trigger->ready()) {
    print_trigger->reset();
  }
}

/** Tell every local LP to start load balancing */
void DistributedScheduler::start_balancing() {
  TW_ASSERT(!running, "Can't balance while scheduler is running\n");
  DEBUG_PE("Starting to load balancing\n");
  for (int i = 0; i < next_lps.get_size(); i++) {
    next_lps.as_array()[i]->lp->load_balance();
  }
}

/** After load balancing completes we can do the next scheduler iteration */
void DistributedScheduler::balancing_complete() {
  DEBUG_PE("Load balancing complete\n");
  lb_trigger->reset();
  next_iteration();
}

#include "conservative.h"
#include "optimistic.h"
#include "scheduler.def.h"
