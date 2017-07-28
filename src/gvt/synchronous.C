#include "synchronous.h"

#include "globals.h"
#include "scheduler.h"
#include "util.h"

SyncGVT::SyncGVT() {
  gvt_name = "Synchronous GVT";
}

void SyncGVT::gvt_begin() {
#ifdef CMK_TRACE_ENABLED
    gvt_start = CmiWallTimer();
#endif
  if(CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_SyncGVT::gvt_contribute(), thisProxy));
  }
}

void SyncGVT::gvt_contribute() {
  Time min_time = scheduler->get_min_time();
  CkAssert(min_time >= curr_gvt);

  contribute(sizeof(Time), &min_time, CkReduction::min_double,
      CkCallback(CkReductionTarget(SyncGVT,gvt_end),thisProxy));

  if(g_tw_async_reduction) {
    scheduler->gvt_resume();
  }
}

void SyncGVT::gvt_end(Time new_gvt) {
  prev_gvt = curr_gvt;
  curr_gvt = new_gvt;
  scheduler->gvt_done(curr_gvt);
#ifdef CMK_TRACE_ENABLED
  traceUserBracketEvent(USER_EVENT_GVT, gvt_start, CmiWallTimer());
#endif
}
