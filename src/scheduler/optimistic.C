#include "optimistic.h"

#include "globals.h"
#include "trigger.h"
#include "avl_tree.h" // Temporary, should be moved to LP
#include "ross_setup.h" // Temporary for AVL_NODE_COUNT

OptimisticScheduler::OptimisticScheduler() {
  scheduler_name = "Optimisitic Scheduler";

  gvt_trigger.reset(new CountTrigger(g_tw_gvt_interval));

  // Initialize the cancel queue
  min_cancel_time = DBL_MAX;
  cancel_q.resize(0);

  // Allocate AVL tree space // TODO: Move this to LPs
  AvlTree avl_list;
  int err = posix_memalign((void **)&avl_list, 64, sizeof(struct avlNode) * AVL_NODE_COUNT);
  memset(avl_list, 0, sizeof(struct avlNode) * AVL_NODE_COUNT);
  for (int i = 0; i < AVL_NODE_COUNT - 1; i++) {
    avl_list[i].next = &avl_list[i + 1];
  }
  avl_list[AVL_NODE_COUNT - 1].next = NULL;
  PE_VALUE(avl_list_head) = &avl_list[0];
  DEBUG_PE("Created AVL tree with %d nodes\n", AVL_NODE_COUNT);
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
  iteration_done();
}

void OptimisticScheduler::gvt_resume() {
  next_iteration();
}

void OptimisticScheduler::gvt_done(Time gvt) {
  collect_fossils(gvt);
  DistributedScheduler::gvt_done(gvt);
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
