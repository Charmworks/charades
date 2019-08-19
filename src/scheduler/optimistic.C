#include "optimistic.h"

#include "avl_tree.h"   // Temporary, should be moved to LP
#include "globals.h"
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
  min_cancel_time = TIME_MAX;

  // Allocate AVL tree space // TODO: Move this to LPs
  AvlTree avl_list;
  int err = posix_memalign((void **)&avl_list, 64,
      sizeof(struct avlNode) * g_event_buffer_size);
  memset(avl_list, 0, sizeof(struct avlNode) * g_event_buffer_size);
  for (int i = 0; i < g_event_buffer_size - 1; i++) {
    avl_list[i].next = &avl_list[i + 1];
  }
  avl_list[g_event_buffer_size - 1].next = NULL;
  PE_VALUE(avl_list_head) = &avl_list[0];
  DEBUG_PE("Created AVL tree with %d nodes\n", g_event_buffer_size);
}

/** Min time for optimistic schedulers must also take cancel q into account */
Time OptimisticScheduler::get_min_time() const {
  if(next_lps.top() != NULL) {
    return std::min(next_lps.top()->ts, min_cancel_time);
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
  iteration_done();
}

void OptimisticScheduler::gvt_resume() {
  next_iteration();
}

void OptimisticScheduler::gvt_done(Time gvt, bool lb) {
  collect_fossils(gvt);
  DistributedScheduler::gvt_done(gvt, lb);
}

void OptimisticScheduler::collect_fossils(Time gvt) {
  for (int i = 0; i < next_lps.get_size(); i++) {
    next_lps.as_array()[i]->lp->fossil_me(gvt);
  }
}

void OptimisticScheduler::balancing_complete() {
  process_cancel_q();
  DistributedScheduler::balancing_complete();
}

/** Call process_cancel_q on every LP chare on our PE */
void OptimisticScheduler::process_cancel_q() {
  min_cancel_time = TIME_MAX;
  for (int i = 0; i < next_lps.get_size(); i++) {
    next_lps.as_array()[i]->lp->process_cancel_q();
  }
}

/** Check for a new min cancel time */
void OptimisticScheduler::update_min_cancel(Time t) {
  if (t < min_cancel_time) {
    min_cancel_time = t;
  }
}
