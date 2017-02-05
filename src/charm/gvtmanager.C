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
GvtSynch::GvtSynch(CProxy_Initialize srcProxy) {}

void GvtSynch::gvt_begin() {

/*  if(waiting_on_gvt) {
    return;
  }
*/
  //iter_cnt = 0;
  //gvt_num++;
 // waiting_on_gvt = true;
  if(CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_GvtSynch::gvt_contribute(), thisProxy)); 
  }
}

void GvtSynch::gvt_contribute() {
  
  GVT gvt_struct;
  //Call Scheduler method to get these values.
  gvt_struct.ts = pes.ckLocalBranch()->get_min_time();
  gvt_struct.type = 0;
  //gvt_struct.type = pes.ckLocalBranch()->get_gvt_type(); 
 // gvt_struct.ts = get_min_time();
 // gvt_struct.type = force_gvt;

//  waiting_on_gvt = false;
  //gvt_started = false;

  //leash_start = get_min_time();
  
  contribute(sizeof(GVT), &gvt_struct, gvtReductionType,
      CkCallback(CkReductionTarget(GvtSynch,gvt_end),thisProxy)); 
}

void GvtSynch::gvt_end(CkReductionMsg* msg) {

  GVT* gvt_struct = (GVT*)msg->getData();
  
  //Call Scheduler gvt_done
  pes.ckLocalBranch()->gvt_done(gvt_struct);
}

/*bool GVT_Synch::wait_on_gvt() {
  return waiting_on_gvt;
}*/

#include "gvtmanager.def.h"
