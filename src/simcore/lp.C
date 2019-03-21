/**
 * \file lp.C
 * Definitions of methods associated with LPs
 */

#include "lp.h"

#include "avl_tree.h"
#include "event.h"
#include "factory.h"
#include "globals.h"
#include "mapper.h"
#include "mpi-interoperate.h"
#include "scheduler.h"
#include "statistics.h"
#include "util.h"

#include <assert.h>
#include <float.h>

/**
 * A readonly declaration for a global lp proxy
 * \todo Re-evaluate need for global/readonly proxies with the new arch
 */
CProxy_LPChare lps;
/**
 * A flag determining whether the lps global proxy is valid. It is needed
 * due to the fact that the array isn't created in a mainchare, so the readonly
 * value is not automatically propogated.
 */
int isLpSet = 0;

/**
 * Calls ckNew to create the LP chare array (only if we are rank 0), and then
 * starts the charm scheduler on every rank.
 */
void create_lps() {
  if (tw_ismaster()) {
    CProxy_LPChare::ckNew(g_num_chares);
  }
  StartCharmScheduler();
}

/**
 * Calls init on the array of LP chares (only if we are rank 0), and then
 * starts the charm scheduler on every rank.
 */
void init_lps() {
  if (tw_ismaster()) {
    lps.init();
  }
  StartCharmScheduler();
}

#undef PE_VALUE
#define PE_VALUE(x) scheduler->globals->x

#undef PE_STATS
#define PE_STATS(x) scheduler->stats->x

/** Initializes all member variables for this LP chare */
LPChare::LPChare() : next_token(this), next_event_id(0), cancel_q(NULL),
            min_cancel_q(TIME_MAX), all_events(0), committed_events(0),
            rolled_back_events(0), committed_time(0), current_time(0) {
  /**
   * Since the lps proxy is not set during main, we have to make sure it is
   * correctly set on every PE during the construction of the array.
   * \todo Remove this if we no longer need a global lp proxy
   */
  if(isLpSet == 0) {
    lps = thisProxy;
    isLpSet = 1;
  }

  usesAtSync = true;
  if (g_tw_ldb_metric != 0) usesAutoMeasure = false;
  isOptimistic = g_tw_synchronization_protocol == OPTIMISTIC;

  /**
   * Get the pointer to the local branch of the scheduler and register our
   * token with it so event execution, fossil collection, and event cancellation
   * can be scheduled for this LP chare.
   */
  scheduler = (Scheduler*)CkLocalBranch(scheduler_id);
  scheduler->register_lp(&next_token, TIME_MAX);

  /**
   * Create the correct number of LPStructs based on the model specified
   * mappings, initialize their RNG streams, but wait to call init handlers.
   */
  lp_structs.resize(g_lp_mapper->get_num_lps(thisIndex));
  for (int i = 0; i < lp_structs.size(); i++) {
    uint64_t gid = g_lp_mapper->get_global_id(thisIndex, i);
    lp_structs[i] = g_lp_factory->create_lp(gid);
    lp_structs[i]->owner = this;
    lp_structs[i]->gid   = gid;

    if (g_tw_rng_default == 1) {
      lp_structs[i]->rng.seed(gid);
    }
  }
}

void LPChare::load_balance() {
  /** Just call AtSync to trigger automatic load balancing */
  AtSync();
}

void LPChare::ResumeFromSync() {
  DEBUG_LP("Now on PE %i\n", CkMyPe());
  /** Do a reduction to inform the scheduler that balancing is complete */
  contribute(
      CkCallback(CkReductionTarget(DistributedScheduler, balancing_complete),
                 CProxy_DistributedScheduler(scheduler_id)));
}

// Helper function for metric determination
/**
 * \name LB Helper Functions
 * Functions for converting timestamps to weights based on the selected metric.
 * Conversions can be inverted to switch what is considered heavy vs light.
 *////@{
/**
 * \param ts timestamp to convert to a weight
 * \returns how close the timestamp is to the simulation end time, as a percent
 */
double tsPercent(Time ts) {
  double weight = std::min((double)ts / g_tw_ts_end, 1.0);
  if (g_tw_metric_invert) return 1.0 - weight;
  else return weight;
}
/**
 * \param ts timestamp to convert to a weight
 * \returns the timestamp bounded by end time, and inverted if required
 */
double tsAbs(Time ts) {
  double weight = std::min(ts, g_tw_ts_end);
  if (g_tw_metric_invert) return g_tw_ts_end - weight;
  else return weight;
}
/**
 * \param ts timestamp to convert to a weight
 * \returns the result of tsAbs or tsPercent based on metric configuration
 */
double timeToWeight(Time ts) {
  if (g_tw_metric_ts_abs) {
    return tsAbs(ts);
  } else {
    return tsPercent(ts);
  }
}
///@}

/**
 * Sets the weight of this LP chare based on an application specific metric. The
 * metric used and how it is used is configured at runtime.
 * \see g_tw_ldb_metric
 * \see g_tw_metric_ts_abs
 * \see g_tw_metric_invert
 */
void LPChare::UserSetLBLoad() {
  double metric;
  switch(g_tw_ldb_metric) {
    case 1:
      if (thisIndex == 0) CkPrintf("Metric: Current Time\n");
      metric = timeToWeight(get_current_time());
      break;
    case 2:
      if (thisIndex == 0) CkPrintf("Metric: Next Event Time\n");
      metric = timeToWeight(events.min());
      break;
    case 3:
      if (thisIndex == 0) CkPrintf("Metric: Latest Commit Time\n");
      metric = timeToWeight(committed_time);
      break;
    case 4:
      if (thisIndex == 0) CkPrintf("Metric: Oldest Event Time\n");
      metric = timeToWeight(processed_events.min());
      break;
    case 5:
      if (thisIndex == 0) CkPrintf("Metric: Weighted Pending Events\n");
      for (int i = 0; i < events.size(); i++) {
        metric += timeToWeight(events.get_temp_event_buffer()[i]->ts);
      }
      break;
    case 6:
      if (thisIndex == 0) CkPrintf("Metric: Committed Events\n");
      metric = committed_events;
      break;
    case 7:
      if (thisIndex == 0) CkPrintf("Metric: Rolled Back Events\n");
      metric = rolled_back_events;
      break;
    case 8:
      if (thisIndex == 0) CkPrintf("Metric: Processed Events\n");
      metric = processed_events.size();
      break;
    case 9:
      if (thisIndex == 0) CkPrintf("Metric: Potential Committed Events\n");
      metric = processed_events.size() + committed_events;
      break;
    case 10:
      if (thisIndex == 0) CkPrintf("Metric: All Executed Events\n");
      metric = processed_events.size() + committed_events + rolled_back_events;
      break;
    case 11:
      if (thisIndex == 0) CkPrintf("Metric: Pending Events\n");
      metric = events.size();
      break;
    case 12:
      if (thisIndex == 0) CkPrintf("Metric: Active Events\n");
      metric = events.size() + processed_events.size();
      break;
    default:
      CkAbort("Invalid load balancing metric\n");
      break;
  }
  DEBUG_LP("On PE %i, Weight: %0.2f\n", CkMyPe(), metric);
  setObjTime(metric);
}

void LPChare::init() {
  /**
   * Call the init handler on every LP struct owned by this chare, then do a
   * reduction to stop the charm scheduler.
   */
  // TODO change this to not the abort event
  PE_VALUE(abort_event)->ts = 0;
  set_current_event(PE_VALUE(abort_event));
  for (int i = 0 ; i < lp_structs.size(); i++) {
    lp_structs[i]->initialize();
  }
  contribute(CkCallback(CkIndex_LPChare::stop_scheduler(), thisProxy(0)));
}

void LPChare::finalize() {
  for (int i = 0; i < lp_structs.size(); i++) {
    lp_structs[i]->finalize();
  }
  contribute(CkCallback(CkReductionTarget(Scheduler, finalize_complete), CProxy_Scheduler(scheduler_id)));
}

void LPChare::stop_scheduler() {
  /**
   * Just call CkExit(), which stops the Charm++ scheduler and returns control
   * to the user defined main function.
   * \note CkCallbacks cannot currently just directly call CkExit because it
   * will exit the whole program instead of just the scheduler.
   * \todo This should be moved to a standalone function at least and maybe even
   * stored in a global CkCallback variable.
   */
  CkExit();
}

void LPChare::recv_remote_event(RemoteEvent* event) {
  /**
   * Inform the scheduler that the remote event was received (which may affect
   * the GVT computation)
   */
  scheduler->consume(event);

  /** Allocate local space for the event and copy over the relevant fields */
  Event *e = event_alloc();
  e->set_msg(event);
  e->state.remote = 1;

  /**
   * In optimistic execution the event needs to be hashed so that it can be
   * found if it needs to be cancelled. Additionally, this checks to see if an
   * anti-event for this event has already arrived in which case the event is
   * immediately freed and ignored.
   */
  if (isOptimistic) {
    Event* anti_event = avlInsertOrDelete(&all_events, e);
    if (anti_event != NULL) {
      tw_event_free(anti_event,false);
      tw_event_free(e,false);
      return;
    }
  }

  /**
   * After pre-processing, the locally allocated event is passed to
   * recv_local_event for the remainder of the work.
   */
  recv_local_event(e);
}

void LPChare::recv_local_event(Event* e) {
  /** Use g_local_map to look up the specific LPStruct pointer for this event */
  e->owner = lp_structs[g_lp_mapper->get_local_id(e->dest_lp)];

  /**
   * If this event is now the earliest event we know about, update the
   * scheduler with our new minumum time.
   */
  if (e->ts < events.min()) {
    scheduler->update_next(&next_token, e->ts);
  }

  /**
   * Check if this event incurs a causality violation
   * \todo Locally sent events can't cause violations so this should be moved
   * to the remote handler.
   */
  if(isOptimistic && e->ts <= get_current_time()) {
    BRACKET_TRACE(rollback_me(e->ts);,USER_EVENT_RB)
  }

  /** Push the event onto our heap of pending events */
  events.push(e);
}

void LPChare::recv_anti_event(RemoteEvent* event) {
  /**
   * Inform the scheduler that the anti event was received (which may affect
   * the GVT computation)
   * \todo This might not be needed once CD is no longer used for GVT
   */
  scheduler->consume(event);

  /** Allocate a local event as a key into the hash and fill it */
  Event* key = event_alloc();
  key->event_id = event->event_id;
  key->ts = event->ts;
  key->src_lp = event->src_lp;

  /**
   * Attempt to insert the key into the AVL hash, which returns an actual event
   * if there was already one that matched the key in the tree.
   */
  Event* real_event = avlInsertOrDelete(&all_events, key);

  /** If there was already a real event, then cancel that event */
  if (real_event != NULL) {
    charm_event_cancel(real_event);
    tw_event_free(key,false);
  }

  delete event;
}

int LPChare::execute_me() {
  if (events.size()) {
    /** Pull off the top event for execution and set the current event and ts */
    Event* e = events.pop();
    set_current_event(e);
    if (isOptimistic) {
      e->cv.clear();
    }
    /** Execute the event on the target LPStruct */
    LPBase* lp = e->owner;
    BRACKET_TRACE(lp->forward(e);, USER_EVENT_FWD)

    /**
     * Move the event to the processed queue if we are optimistic, otherwise
     * free and commit the event immediately.
     */
    if (isOptimistic) {
      processed_events.push_front(e);
    } else {
      tw_event_free(e,true);
      committed_events++;
      committed_time = e->ts;
    }

    /** Update the scheduler with our new minimum timestamp */
    scheduler->update_next(&next_token, events.min());
    return 1;
  }
  return 0;
}

void LPChare::rollback_me(Time ts) {
  Event* e;
  PE_STATS(total_rollback_calls)++;
  PE_STATS(ts_rollback_calls)++;
  /**
   * Pop events from our processed queue and roll them back until the most
   * recently processed event has a timestamp less than or equal to ts. Each
   * rolled back event is pushed back onto the pending heap.
   */
  while(processed_events.size() && processed_events.front()->ts >= ts) {
    e = processed_events.pop_front();
    tw_event_rollback(e);
    rolled_back_events++;
    events.push(e);
  }

  /**
   * Update our min time in the scheduler, and current event and time to point
   * to our new most recently processed event, or NULL if there are no longer
   * any processed events.
   */
  scheduler->update_next(&next_token, events.min());
  set_current_event(processed_events.front());
}

void LPChare::rollback_me(Event* event) {
  PE_STATS(total_rollback_calls)++;
  PE_STATS(event_rollback_calls)++;

  /**
   * Pop events from our processed queue until we pop an event matching the
   * event passed in. For all events that don't match, we roll them back and
   * push them back onto the pending heap.
   */
  Event* e = processed_events.pop_front();
  while (e != event) {
    tw_event_rollback(e);
    rolled_back_events++;
    events.push(e);
    e = processed_events.pop_front();
  }

  /**
   * Once we have found the event that was passed in, we roll it back as normal
   * but then return control to the calling context, which will decide how to
   * proceed.
   */
  assert(e == event);
  tw_event_rollback(event);
  rolled_back_events++;

  /**
   * Update our min time in the scheduler, and current event and time to point
   * to our new most recently processed event, or NULL if there are no longer
   * any processed events.
   */
  scheduler->update_next(&next_token, events.min());
  set_current_event(processed_events.front());
}

void LPChare::fossil_me(Time gvt) {
  /**
   * Pop off events from the BACK of the processed queue and commit/free them
   * until the oldest timestamp in the processed queue is greater than or equal
   * to the passed in GVT.
   */
  Event* e;
  while (processed_events.back() != NULL && processed_events.back()->ts < gvt) {
    e = processed_events.pop_back();
    tw_event_free(e,true);
    committed_events++;
    committed_time = e->ts;
  }
}

void LPChare::cancel_event(Event* e) {
  /** \pre the event e exists in either the pending heap or processed queue */
  /** Call a different method based on where e currently exists */
  switch (e->state.owner) {
    /** If the event hasn't been executed yet, just delete from pending heap */
    case TW_chare_q:
      delete_pending(e);
      tw_event_free(e,false);
      return;
    /** If the event was alredy executed, add it to the cancel queue */
    case TW_rollback_q:
      add_to_cancel_q(e);
      return;
    /** If the event exists somewhere else, this is an error */
    default:
      CkAbort("Unknown owner in LPChare::cancel_event\n");
      return;
  }
}

void LPChare::add_to_cancel_q(Event* e) {
  /** Put the event at the head of the cancel queue */
  e->state.cancel_q = 1;
  e->cancel_next = cancel_q;
  cancel_q = e;
  /** Update the min cancel time locally and in the scheduler if necessary */
  if (e->ts < min_cancel_q) {
    min_cancel_q = e->ts;
    scheduler->update_min_cancel(min_cancel_q);
  }
}

void LPChare::delete_pending(Event *e) {
  /** Must also update the min time of this LP chare in the scheduler */
  events.erase(e);
  scheduler->update_next(&next_token, events.min());
}

void LPChare::process_cancel_q() {
  /**
   * Iterate through cancel queue and cancel every event based on where it
   * currently exists.
   *
   * \note Cancelling an event in the queue can cause a new even to be added
   * to the queue which is why a double loop is required.
   *
   * \note Even though only events in the processed queue are placed in the
   * cancel queue, they may be rolled back via other means and therefore end
   * up in the pending heap for this step.
   */
  Event *curr, *next;
  while (cancel_q) {
    curr = cancel_q;
    cancel_q = NULL;
    min_cancel_q = TIME_MAX;

    /**
     * For each event in the queue, cancel it based on it's current location.
     */
    while(curr) {
      next = curr->cancel_next;
      switch (curr->state.owner) {
        /** If the event hasn't been executed yet, just delete from pending */
        case TW_chare_q:
          delete_pending(curr);
          tw_event_free(curr,false);
          break;

        case TW_rollback_q:
          /** If the event was alredy executed, roll back to it and free it */
          rollback_me(curr);
          tw_event_free(curr,false);
          break;

          /** If the event exists somewhere else, this is an error */
        default:
          CkAbort("Unknown event owner in cancel_q\n");
          break;
      }
      curr = next;
    }
  }
}

#include "lp.def.h"
