#include "gvt_manager.h"
#include "lp.h"
#include "charm_functions.h"

#include "ross_util.h"
#include "ross_api.h"

#include "mpi-interoperate.h"

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


/* General GVT Functions */
/*
void GVT_Manager::gvt_print(GVT* gvt_struct) {
  if (gvt_print_interval == 1.0) {
    return;
  }
  if (PE_VALUE(percent_complete) == 0.0) {
    PE_VALUE(percent_complete) = gvt_print_interval;
    return;
  }
  CkPrintf("GVT #%d", gvt_num);
  if (gvt_struct->type) {
    CkPrintf(" (FORCED %d)", gvt_struct->type);
  }
  CkPrintf(": simulation %d%% complete ",
      (int)fmin(100, floor(100 * (gvt_struct->ts/g_tw_ts_end))));
  if (gvt_struct->ts == DBL_MAX) {
    CkPrintf("(GVT = MAX).\n");
  } else {
    CkPrintf("(GVT = %.4f).\n", gvt_struct->ts);
  }
  PE_VALUE(percent_complete) += gvt_print_interval;
}

*/

/* GVT SYNCH FUNCTIONS */
GVT_Synch::GVT_Synch(): waiting_on_gvt(false), gvt_started(false) {}

void GVT_Synch::gvt_begin() {

  if(waiting_on_gvt) {
    return;
  }

  //iter_cnt = 0;
  //gvt_num++;
  waiting_on_gvt = true;
  if(CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_GVT_Synch::gvt_contribute(), thisProxy)); 
  }
}

void GVT_Synch::gvt_contribute() {
  
  GVT gvt_struct;
  //Call Scheduler method to get these values.
 // gvt_struct.ts = get_min_time();
 // gvt_struct.type = force_gvt;

  waiting_on_gvt = false;
  gvt_started = false;

  //leash_start = get_min_time();
  
  contribute(sizeof(GVT), &gvt_struct, gvtReductionType,
      CkCallback(CkReductionTarget(GVT_Synch,gvt_end),thisProxy)); 
}

void GVT_Synch::gvt_end(CkReductionMsg* msg) {

  GVT* gvt_struct = (GVT*)msg->getData();
  
  //Call Scheduler gvt_done

}

bool GVT_Synch::wait_on_gvt() {
  return waiting_on_gvt;
}

#include "gvt_handler.def.h"
