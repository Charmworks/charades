#include "scheduler.h"

#include "charm_functions.h"
#include "gvtmanager.h"
#include "lp.h"
#include "ross.h"
#include "globals.h"
#include "ross_random.h"

#include <float.h> // Included for DBL_MAX

CProxy_Scheduler scheduler_proxy;

/******************************************************************************/
/* Scheduler Base Class                                                       */
/******************************************************************************/
Scheduler::Scheduler() {
  globals = new Globals();
  stats = new Statistics();

  contribute(CkCallback(CkReductionTarget(Scheduler, schedulerReady), thisProxy));
  thisProxy[CkMyPe()].initialize();
}

void Scheduler::initialize_rand() {
  DEBUG_PE("Random number generator initialized\n");
  rng = tw_rand_init(31, 41);
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
  CkAbort("SequentialScheduler not yet implemented\n");
}

/******************************************************************************/
/* Conservative Scheduler                                                     */
/******************************************************************************/
ConservativeScheduler::ConservativeScheduler() {
  scheduler_name = "Conservative Scheduler";
}

/** Only execute events within 'lookahead' time of current GVT */
void ConservativeScheduler::execute() {
  while (get_min_time() < gvt_manager->current_gvt() + g_tw_lookahead) {
    if (!schedule_next_lp()) {
      break;
    }
  }
  gvt_manager->gvt_begin();
}

/******************************************************************************/
/* Optimistic Scheduler                                                       */
/******************************************************************************/
OptimisticScheduler::OptimisticScheduler() : trigger(8),
                                             min_cancel_time(DBL_MAX) {
  scheduler_name = "Optimisitic Scheduler";
  cancel_q.resize(0);
}

/** Min time for optimistic schedulers must also take cancel q into account */
Time OptimisticScheduler::get_min_time() const {
  if(next_lps.top() != NULL) {
    return fmin(next_lps.top()->ts, min_cancel_time);
  } else {
    return min_cancel_time;
  }
}

/** Execute events in batches until the trigger dictates it's GVT time */
void OptimisticScheduler::execute() {
  for (int num_executed = 0; num_executed < g_tw_mblock; num_executed++) {
    if (!schedule_next_lp()) {
      break;
    }
  }
  process_cancel_q();
  trigger.iteration_done();
  if (trigger.is_ready(get_min_time())) {
    gvt_manager->gvt_begin();
  } else {
    thisProxy[CkMyPe()].execute();
  }
}

void OptimisticScheduler::gvt_resume() {
  thisProxy[CkMyPe()].execute();
  Scheduler::gvt_resume();
}

void OptimisticScheduler::gvt_done(Time gvt) {
  trigger.gvt_done(gvt);
  collect_fossils(gvt);
  Scheduler::gvt_done(gvt);
}

/** Call fossil_me on all lps that have fossils older than the current gvt. The
 *  oldest_lps queue ensures we will only call fossil_me on lps that need it. */
void OptimisticScheduler::collect_fossils(Time gvt) {
  /*LPToken *min = oldest_lps.top();
  while((min != NULL) && (min->ts < gvt)) {
    PE_STATS(fossil_collect_calls)++;
    min->lp->fossil_me(gvt);
    min = oldest_lps.top();
  }*/
  //iterate over all lps and fossil collect
  for (int i = 0; i < next_lps.get_size(); i++) {
    next_lps.as_array()[i]->lp->fossil_me(gvt);
  }
}

/** Call process_cancel_q on every LP chare in our PE level cancel_q */
void OptimisticScheduler::process_cancel_q() {
  vector<LP*> temp_q;
  temp_q.swap(cancel_q);
  min_cancel_time = DBL_MAX;

  for(int i = 0; i < temp_q.size(); i++) {
    temp_q[i]->process_cancel_q();
  }
}

/** Add an lp to the cancel queue and check for a new min time */
void OptimisticScheduler::add_to_cancel_q(LP* lp) {
  cancel_q.push_back(lp);
  if (lp->min_cancel_time() < min_cancel_time) {
    min_cancel_time = lp->min_cancel_time();
  }
}

/** Check for a new min cancel time */
void OptimisticScheduler::update_min_cancel(Time t) {
  if (t < min_cancel_time) {
    min_cancel_time = t;
  }
}

#include "scheduler.def.h"
