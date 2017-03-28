#include "conservative.h"

#include "globals.h"
#include "trigger.h"

ConservativeScheduler::ConservativeScheduler() {
  scheduler_name = "Conservative Scheduler";
  gvt_trigger.reset(new ConstTrigger(true));
}

/** Only execute events within 'lookahead' time of current GVT */
void ConservativeScheduler::execute() {
  while (get_min_time() < gvt_manager->current_gvt() + g_tw_lookahead) {
    if (!schedule_next_lp()) {
      break;
    }
  }
  iteration_done();
}

