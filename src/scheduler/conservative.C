#include "conservative.h"

#include "globals.h"

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

