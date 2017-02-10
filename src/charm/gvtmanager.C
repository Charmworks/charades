#include "gvtmanager.h"

#include "charm_functions.h"
#include "pe.h"
#include "scheduler.h"
 
CProxy_GVTManager gvt_manager_proxy;
CkReduction::reducerType gvtReductionType;

/* NON-MEMBER functions */
CkReductionMsg *gvtReduction(int nMsg, CkReductionMsg **msgs) {
  GVT* new_gvt = new GVT;
  for (int i = 0; i < nMsg; i++) {
    CkAssert(msgs[i]->getSize() == sizeof(GVT));
    GVT* gvt = (GVT*)msgs[i]->getData();
    new_gvt->ts = fmin(new_gvt->ts, gvt->ts);
    new_gvt->type = new_gvt->type | gvt->type;
  }
  return CkReductionMsg::buildNew(sizeof(GVT), new_gvt);
}

void registerGVTReduction(void) {
  gvtReductionType = CkReduction::addReducer(gvtReduction);
}

/* GVTManager FUNCTIONS */
GVTManager::GVTManager() : gvt(0.0) {
  contribute(CkCallback(CkReductionTarget(PEManager, gvtManagerReady), pe_manager_proxy));
}

/* GVT SYNC FUNCTIONS */
SyncGVT::SyncGVT() {}

void SyncGVT::gvt_begin() {
  if(CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_SyncGVT::gvt_contribute(), thisProxy));
  }
}

void SyncGVT::gvt_contribute() {
  Time min_time = scheduler->get_min_time();
  CkAssert(min_time >= gvt);
  
  contribute(sizeof(Time), &min_time, CkReduction::min_double,
      CkCallback(CkReductionTarget(SyncGVT,gvt_end),thisProxy));

  if(g_tw_async_reduction) {
    scheduler->gvt_resume();
  }
}

void SyncGVT::gvt_end(Time new_gvt) {
  gvt = new_gvt;
  scheduler->gvt_done(gvt);
}

#include "gvtmanager.def.h"
