#include "synchronous.h"

#include "globals.h"
#include "scheduler.h"
#include "trigger.h"
#include "util.h"

SyncGVT::SyncGVT() {
  gvt_name = "Synchronous GVT";
  active = false;
  continuous = false;
}

void SyncGVT::gvt_begin() {
  active = true;
#ifdef CMK_TRACE_ENABLED
    gvt_start = CmiWallTimer();
#endif
  if(CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_SyncGVT::gvt_contribute(), thisProxy));
  }

  lb_trigger->iteration_done();
}

void SyncGVT::gvt_contribute() {
  Time min_time = scheduler->get_min_time();
  CkAssert(min_time >= curr_gvt);

  contribute(sizeof(Time), &min_time, CkReduction::min_ulong_long,
      CkCallback(CkReductionTarget(SyncGVT,gvt_end),thisProxy));

  if(g_tw_async_reduction && !lb_trigger->ready()) {
    scheduler->gvt_resume();
  }
}

void SyncGVT::gvt_end(Time new_gvt) {
  active = false;
  prev_gvt = curr_gvt;
  curr_gvt = new_gvt;

  if (lb_trigger->ready()) {
    lb_trigger->reset();
    scheduler->gvt_done(curr_gvt, true);
  } else {
    scheduler->gvt_done(curr_gvt, false);
  }
#ifdef CMK_TRACE_ENABLED
  traceUserBracketEvent(USER_EVENT_GVT, gvt_start, CmiWallTimer());
#endif
}
