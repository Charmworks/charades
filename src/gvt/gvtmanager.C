#include "gvtmanager.h"

#include "globals.h"
#include "scheduler.h"
#include "trigger.h"
#include "util.h"

CProxy_GVTManager gvt_manager_proxy;

/* GVTManager FUNCTIONS */
GVTManager::GVTManager() : curr_gvt(0.0), prev_gvt(0.0) {
  gvt_manager_proxy = thisProxy;

  // Set up the load balancing trigger
  if (g_tw_ldb_interval > 0 || g_tw_ldb_first > 0) {
    if (g_tw_ldb_interval == 0) {
      g_tw_max_ldb = 1;
    } else if (g_tw_ldb_first == 0) {
      g_tw_ldb_first = g_tw_ldb_interval;
    }
    lb_trigger.reset(new CountTrigger(g_tw_ldb_first, g_tw_ldb_interval));
    if (g_tw_max_ldb > 0) {
      lb_trigger.reset(new BoundedTrigger(lb_trigger.release(), g_tw_max_ldb));
    }
  } else {
    lb_trigger.reset(new ConstTrigger(false));
  }
}

void GVTManager::groups_created() {
  DEBUG_PE("%s created!\n", gvt_name.c_str());
  scheduler = (DistributedScheduler*)CkLocalBranch(scheduler_id);
}

#include "synchronous.h"
#include "cd.h"
#include "phased.h"
#include "bucketed.h"
#include "gvtmanager.def.h"
