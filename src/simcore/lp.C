#include "lp.h"

#include "avl_tree.h"
#include "event.h"
#include "globals.h"
#include "mpi-interoperate.h"
#include "scheduler.h"
#include "statistics.h"
#include "ross_random.h"
#include "ross_clcg4.h"
#include "util.h"

#include <assert.h>
#include <float.h>

CProxy_LP lps;
int isLpSet = 0;

// TODO: Move this API to an appropriate place
void create_lps() {
  if (tw_ismaster()) {
    CProxy_LP::ckNew(g_num_chares);
  }
  StartCharmScheduler();
}

void init_lps() {
  if (tw_ismaster()) {
    lps.init();
  }
  StartCharmScheduler();
}

Time tw_now(tw_lp* lp) {
  return lp->owner->current_time;
}

Event* current_event(tw_lp* lp) {
  return lp->owner->current_event;
}

void set_current_event(tw_lp* lp, Event* event) {
  lp->owner->current_event = event;
  lp->owner->current_time = event->ts;
}

#undef PE_VALUE
#define PE_VALUE(x) scheduler->globals->x

#undef PE_STATS
#define PE_STATS(x) scheduler->stats->x

// Create LPStructs based on mappings, and do initial registration with the PE.
LP::LP() : next_token(this), uniqID(0), min_cancel_q(DBL_MAX),
           current_time(0), all_events(0), committed_events(0),
           rolled_back_events(0), committed_time(0.0), latest_time(0.0) {
  if(isLpSet == 0) {
    lps = thisProxy;
    isLpSet = 1;
  }
  usesAtSync = true;

  if (g_tw_ldb_metric != 0) usesAutoMeasure = false;

  // Cache the pointer to the local PE chare
  scheduler = (Scheduler*)CkLocalBranch(scheduler_id);

  // Register with the local PE so it can schedule this LP for execution, fossil
  // collection, and cancelation.
  scheduler->register_lp(this, &next_token, 0.0);

  isOptimistic = g_tw_synchronization_protocol == OPTIMISTIC;

  // Create array of LPStructs based on globals and initialize RNG streams
  lp_structs.resize(g_numlp_map(thisIndex));
  for (int i = 0; i < lp_structs.size(); i++) {
    lp_structs[i].owner = this;
    lp_structs[i].gid   = g_init_map(thisIndex, i);
    lp_structs[i].type  = g_type_map(lp_structs[i].gid);
    lp_structs[i].state = malloc(lp_structs[i].type->state_size);

    if (g_tw_rng_default == 1) {
      tw_rand_init_streams(&lp_structs[i], g_tw_nRNG_per_lp);
    }
  }
}

void LP::load_balance() {
  AtSync();
}

void LP::ResumeFromSync() {
  DEBUG_LP("Now on PE %i\n", CkMyPe());
  contribute(
      CkCallback(CkReductionTarget(DistributedScheduler, balancing_complete),
                 CProxy_DistributedScheduler(scheduler_id)));

  // Reset manually tracked metrics for next LB period
  committed_events = rolled_back_events = 0;
  latest_time = committed_time = 0.0;
}

// Helper function for metric determination
double tsPercent(Time ts) {
  double weight = fmin(ts / g_tw_ts_end,1.0);
  if (g_tw_metric_invert) return 1.0 - weight;
  else return weight;
}
double tsAbs(Time ts) {
  double weight = fmin(ts, g_tw_ts_end);
  if (g_tw_metric_invert) return g_tw_ts_end - weight;
  else return weight;
}
double timeToWeight(Time ts) {
  if (g_tw_metric_ts_abs) {
    return tsAbs(ts);
  } else {
    return tsPercent(ts);
  }
}

void LP::UserSetLBLoad() {
  double metric;
  switch(g_tw_ldb_metric) {
    case 1:
      if (thisIndex == 0) CkPrintf("Metric: Current Time\n");
      metric = timeToWeight(current_time);
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
    case 13:
      if (thisIndex == 0) CkPrintf("Metric: Latest Time\n");
      metric = timeToWeight(latest_time);
      break;
    default:
      CkAbort("Invalid load balancing metric\n");
      break;
  }
  DEBUG_LP("On PE %i, Weight: %0.2f\n", CkMyPe(), metric);
  setObjTime(metric);
}

// Call init on all LPs then stop the charm scheduler.
void LP::init() {
  current_event = PE_VALUE(abort_event);
  for (int i = 0 ; i < lp_structs.size(); i++) {
    lp_structs[i].type->init(lp_structs[i].state, &lp_structs[i]);
  }
  contribute(CkCallback(CkIndex_LP::stop_scheduler(), thisProxy(0)));
}

void LP::stop_scheduler() {
  CkExit();
}

// Entry method for receiving remote events.
// 1) Allocate a new event and fill it based on the remote event.
// 2) Hash the event if optimistic.
// 3) Pass control to the local receive method.
void LP::recv_remote_event(RemoteEvent* event) {
  scheduler->consume(event);
  Event *e = charm_allocate_event(0);
  e->state.remote = 1;

  // Fill in event
  e->eventMsg = event;
  e->event_id = event->event_id;
  e->ts       = event->ts;
  e->send_pe  = event->send_pe;
  e->dest_lp  = event->dest_lp;
  e->userData = event->userData;

  // Hash event
  if (isOptimistic) {
    Event* anti_event = avlInsertOrDelete(&all_events, e);
    if (anti_event != NULL) {
      tw_event_free(anti_event,false);
      tw_event_free(e,false);
      return;
    }
  }

  recv_local_event(e);
}

// Local method for receiving an event.
// 1) Get the local lp_struct pointer.
// 2) Do any required PE updates or rollbacks.
// 3) Push event into the priority queue.
void LP::recv_local_event(Event* e) {
  e->dest_lp = (tw_lpid)&lp_structs[g_local_map(e->dest_lp)];

  if(isOptimistic && e->ts < current_time) {
    BRACKET_TRACE(rollback_me(e->ts);,USER_EVENT_RB)
  }
  if (e->ts < events.min()) {
    scheduler->update_next(&next_token, e->ts);
  }

  events.push(e);
}

// Entry method for receiving anti events.
// 1) Create a key event based on the remote event.
// 2) Use the key to find the real event and cancel it.
void LP::recv_anti_event(RemoteEvent* event) {
  scheduler->consume(event);
  Event* key = charm_allocate_event(0);
  key->event_id = event->event_id;
  key->ts = event->ts;
  key->send_pe = event->send_pe;

  Event* real_event = avlInsertOrDelete(&all_events, key);

  if (real_event != NULL) {
    charm_event_cancel(real_event);
    tw_event_free(key,false);
  }

  delete event;
}

// Execute the next event in the pending queue (returns false if no events).
// 1) Pop event
// 2) Execute event
// 3) Free event, or put into processed queue if optimistic
// 4) Update the PE with our new earliest timestamp
void* LP::execute_me() {
  if (events.size()) {
    // Pull off the top event for execution
    Event* e = events.pop();
    current_time = e->ts;
    current_event = e;
    latest_time = std::max(latest_time, current_time);
    LPStruct* lp = (LPStruct*)e->dest_lp;
    if (isOptimistic) {
      reset_bitfields(e);
    }
    BRACKET_TRACE(lp->type->execute(lp->state, &e->cv, tw_event_data(e), lp);, USER_EVENT_FWD)

    // Enqueue or deallocate the event depending on sync mode
    if (isOptimistic) {
      processed_events.push_front(e);
    } else {
      tw_event_free(e,true);
      committed_events++;
      committed_time = e->ts;
    }
    scheduler->update_next(&next_token, events.min());
    return (void*)true;
  }
  return (void*)false;
}

// Rollback all processed events up to the passed in timestamp.
void LP::rollback_me(tw_stime ts) {
  Event* e;
  PE_STATS(total_rollback_calls)++;
  PE_STATS(ts_rollback_calls)++;
  while(processed_events.size() && processed_events.front()->ts > ts) {
    e = processed_events.pop_front();
    tw_event_rollback(e);
    rolled_back_events++;
    events.push(e);
  }

  scheduler->update_next(&next_token, events.min());
  if(processed_events.front() == NULL) {
    current_event = NULL;
    current_time = PE_VALUE(g_last_gvt);
  } else {
    current_event = processed_events.front();
    current_time = current_event->ts;
  }
}

// Rollback until we get to event, and roll it back.
void LP::rollback_me(Event *event) {
  PE_STATS(total_rollback_calls)++;
  PE_STATS(event_rollback_calls)++;
  Event* e = processed_events.pop_front();
  while (e != event) {
    tw_event_rollback(e);
    rolled_back_events++;
    events.push(e);
    e = processed_events.pop_front();
  }
  // We've found the event in question so roll it back.
  // The caller will correctly handle what else to do with the event.
  assert(e == event);
  tw_event_rollback(event);
  rolled_back_events++;

  // Update the queues, and current variables.
  scheduler->update_next(&next_token, events.min());
  if(processed_events.front() == NULL) {
    current_event = NULL;
    current_time = PE_VALUE(g_last_gvt);
  } else {
    current_event = processed_events.front();
    current_time = current_event->ts;
  }
}

// Fossil collect all events older than the passed in GVT.
// 1) If the next event is older than the current gvt pop it and delete it.
// 2) Update the PE with our oldest unprocessed event time.
void LP::fossil_me(tw_stime gvt) {
  Event* e;
  while (processed_events.size() && processed_events.back()->ts < gvt) {
    e = processed_events.pop_back();
    tw_event_free(e,true);
    committed_events++;
    committed_time = e->ts;
  }
}

// Cancel the event e, which should be stored on this chare.
void LP::cancel_event(Event* e) {
  switch (e->state.owner) {
    case TW_chare_q:
      // If the event hasn't been executed, just free it
      delete_pending(e);
      tw_event_free(e,false);
      return;
    case TW_rollback_q:
      // If the event has already been executed, add it to the cancel_q
      add_to_cancel_q(e);
      return;
    default:
      CkAbort("Unknown owner in LP::cancel_event\n");
      return;
  }
}

void LP::add_to_cancel_q(Event* e) {
  cancel_queue.push_back(e);
  e->state.cancel_q = 1;
  if (e->ts < min_cancel_q) {
    min_cancel_q = e->ts;
    scheduler->update_min_cancel(min_cancel_q);
  }
}

// Delete an event in our pending queue.
void LP::delete_pending(Event *e) {
  events.erase(e);
  scheduler->update_next(&next_token, events.min());
}

// Cancel all events in the cancel queue.
// NOTE: Even though only events in the rollback queue are place in the cancel
// queue, they can be rolled back during execution and end up back in the
// chare queue.
void LP::process_cancel_q() {
  while (cancel_queue.size()) {
    Event* e = cancel_queue.front();
    cancel_queue.pop_front();

    switch (e->state.owner) {
      case TW_chare_q:
        delete_pending(e);
        tw_event_free(e,false);
        break;

      case TW_rollback_q:
        rollback_me(e);
        tw_event_free(e,false);
        break;

      default:
        CkPrintf("[%i,%i]: Unknown event owner in cancel queue%i\n",
            CkMyPe(), e->state.owner);
        CkAbort("ERROR: Unable to process cancel queue\n");
        break;
    }
  }
  min_cancel_q = DBL_MAX;
  TW_ASSERT(cancel_queue.size() == 0, "Non-empty cancel queue\n");
}

#include "lp.def.h"
