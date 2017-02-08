#include "gvtmanager.h"
#include "scheduler.h"
#include "charm_functions.h"
 
#include "ross_util.h"
#include "ross_api.h"

#include "mpi-interoperate.h"

CProxy_GvtManager gvts;
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

/* GvtManager FUNCTIONS */
GvtManager::GvtManager() : gvt(0.0) {}

/* GVT SYNC FUNCTIONS */
GvtSync::GvtSync() {}

void GvtSync::gvt_begin() {
  if(CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_GvtSync::gvt_contribute(), thisProxy)); 
  }
}

void GvtSync::gvt_contribute() {
  GVT gvt_struct;
  gvt_struct.ts = scheduler.ckLocalBranch()->get_min_time();
  gvt_struct.type = 0;
  CkAssert(gvt_struct.ts >= gvt);
  
  contribute(sizeof(GVT), &gvt_struct, gvtReductionType,
      CkCallback(CkReductionTarget(GvtSync,gvt_end),thisProxy)); 

  if(g_tw_async_reduction) {}
}

void GvtSync::gvt_end(CkReductionMsg* msg) {
  GVT* gvt_struct = (GVT*)msg->getData();
  gvt = gvt_struct->ts;
  scheduler.ckLocalBranch()->gvt_done(gvt);
}

#include "gvtmanager.def.h"
