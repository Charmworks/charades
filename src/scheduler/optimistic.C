#include "optimistic.h"

#include "avl_tree.h"   // Temporary, should be moved to LP
#include "globals.h"
#include "statistics.h"
#include "trigger.h"
#include "util.h"

OptimisticScheduler::OptimisticScheduler() {
  scheduler_name = "Optimisitic Scheduler";

  // Set up the appropriate GVT trigger
  if (g_tw_gvt_trigger == 1) {
    gvt_trigger.reset(new CountTrigger(g_tw_gvt_interval));
  } else if (g_tw_gvt_trigger == 2) {
    gvt_trigger.reset(new LeashTrigger(this, g_tw_gvt_interval));
  } else {
    CkAbort("Bad trigger value\n");
  }

  // Initialize the cancel queue
  min_cancel_time = DBL_MAX;

  // Allocate AVL tree space // TODO: Move this to LPs
  AvlTree avl_list;
  int err = posix_memalign((void **)&avl_list, 64,
      sizeof(struct avlNode) * g_tw_max_events_buffered);
  memset(avl_list, 0, sizeof(struct avlNode) * g_tw_max_events_buffered);
  for (int i = 0; i < g_tw_max_events_buffered - 1; i++) {
    avl_list[i].next = &avl_list[i + 1];
  }
  avl_list[g_tw_max_events_buffered - 1].next = NULL;
  PE_VALUE(avl_list_head) = &avl_list[0];
  DEBUG_PE("Created AVL tree with %d nodes\n", g_tw_max_events_buffered);
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
#ifdef DETAILED_TIMING
  double start = CmiWallTimer();
#endif
  for (int num_executed = 0; num_executed < g_tw_mblock; num_executed++) {
    if (!schedule_next_lp()) {
      break;
    }
  }
#ifdef DETAILED_TIMING
  double execute_time = CmiWallTimer() - start;
  PE_STATS(execute_time) += execute_time;
#endif
  process_cancel_q();
  iteration_done();
}

void OptimisticScheduler::gvt_resume() {
#ifdef DETAILED_TIMING
  double gvt_delay = CmiWallTimer() - gvt_start;
  PE_STATS(gvt_delay) += gvt_delay;
  delay_marked = true;
#endif
  PE_VALUE(g_leash_time) = get_min_time();
  next_iteration();
}

void OptimisticScheduler::gvt_done(Time gvt, bool lb) {
  collect_fossils(gvt);
  DistributedScheduler::gvt_done(gvt, lb);
}

void OptimisticScheduler::collect_fossils(Time gvt) {
#ifdef DETAILED_TIMING
  double fossil_time = CmiWallTimer();
#endif
  for (LP* lp : registered_lps) {
    lp->fossil_me(gvt);
  }
#ifdef DETAILED_TIMING
  fossil_time = CmiWallTimer() - fossil_time;
  PE_STATS(fossil_time) += fossil_time;
#endif
}
}

/** Call process_cancel_q on every LP chare on our PE */
void OptimisticScheduler::process_cancel_q() {
  for (LP* lp : registered_lps) {
    lp->process_cancel_q();
  }
  min_cancel_time = DBL_MAX;
}

/** Check for a new min cancel time */
void OptimisticScheduler::update_min_cancel(Time t) {
  if (t < min_cancel_time) {
    min_cancel_time = t;
  }
}
