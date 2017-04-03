#include "gvtmanager.h"

#include "scheduler.h"

CProxy_GVTManager gvt_manager_proxy;

/* GVTManager FUNCTIONS */
GVTManager::GVTManager() : curr_gvt(0.0), prev_gvt(0.0) {
  gvt_manager_proxy = thisProxy;
}

void GVTManager::groups_created() {
  scheduler = (DistributedScheduler*)CkLocalBranch(scheduler_id);
}

#include "synchronous.h"
#include "cd.h"
#include "phased.h"
#include "bucketed.h"
#include "gvtmanager.def.h"
