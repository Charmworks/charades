#include "gvtmanager.h"
#include "lp.h"
#include "charm_functions.h"
#include "pe.h"
 
#include "ross_util.h"
#include "ross_api.h"

#include "mpi-interoperate.h"

CProxy_GvtManager gvts;
extern CProxy_PE pes;
CkReduction::reducerType gvtReductionType;

/* NON-MEMBER functions */

void registerGVTReduction(void) {
  gvtReductionType = CkReduction::addReducer(gvtReduction);
}


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


/* GvtManager FUNCTIONS */
GvtManager::GvtManager() {}
GvtManager::GvtManager(CProxy_Initialize srcProxy) {}

void GvtManager::gvt_begin() {}



/* GVT SYNCH FUNCTIONS */
GvtSynch::GvtSynch(CProxy_Initialize srcProxy) {}

void GvtSynch::gvt_begin() {

  if(CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_GvtSynch::gvt_contribute(), thisProxy)); 
  }
}

void GvtSynch::gvt_contribute() {
  
  GVT gvt_struct;
  //Call Scheduler method to get these values.
  gvt_struct.ts = pes.ckLocalBranch()->get_min_time();
  //TODO: Change this type??
  gvt_struct.type = 0;
  
  contribute(sizeof(GVT), &gvt_struct, gvtReductionType,
      CkCallback(CkReductionTarget(GvtSynch,gvt_end),thisProxy)); 
}

void GvtSynch::gvt_end(CkReductionMsg* msg) {

  GVT* gvt_struct = (GVT*)msg->getData();
  
  //Call Scheduler gvt_done
  pes.ckLocalBranch()->gvt_done(gvt_struct);
}

#include "gvtmanager.def.h"
