#include "pe.h"
#include "ross_util.h"
#include "ross_api.h"
#include "mpi-interoperate.h"
#include "float.h"

CProxy_PE pes;

// This is the API which allows the ROSS code to initialize and access the
// Charm++ backend.

Globals* get_globals() {
  return pes.ckLocalBranch()->globals;
}

// Starts the simulation by calling the scheduler on all pes
void charm_run() {
  if(tw_ismaster()) DEBUG("[%d] Initializing schedulers \n", CkMyPe());
  if (tw_ismaster()) {
    if(PE_VALUE(g_tw_synchronization_protocol) == SEQUENTIAL) {
      pes.execute_seq();
    } if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
      pes.execute_cons();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      pes.execute_opt();
    } else {
      tw_error(TW_LOC, "Incorrect scheduler values, Aborting\n");
    }
  }
  StartCharmScheduler();
}

PE::PE(CProxy_Initialize srcProxy) : gvt_cnt(0) {
  globals = new Globals;
  globals->g_lps_per_chare = 4;
  globals->g_tw_synchronization_protocol = CONSERVATIVE;
  globals->g_tw_ts_end = 100000;
  globals->g_tw_mblock = 16;
  globals->g_tw_gvt_interval = 16;
  globals->g_tw_nlp = globals->g_lps_per_chare;
  globals->g_tw_rng_seed = NULL;
  globals->g_tw_rng_max = 1;
  globals->g_tw_nRNG_per_lp = 1;
  globals->g_tw_rng_default = 1;
  globals->g_tw_csv = NULL;
  globals->g_num_lp_chares = CkNumPes();
  globals->g_tw_max_events_buffered = 1000;
  globals->g_tw_min_detected_offset = DBL_MAX;
  globals->g_tw_lookahead = .005;
  globals->g_init_map = init_block_map;
  globals->g_local_map = local_block_map;
  globals->lastGVT = 0.0;
  gvt = 0.0;
  thisProxy[CkMyPe()].initialize_rand(srcProxy);
}

void PE::initialize_rand(CProxy_Initialize srcProxy) {
  rng = tw_rand_init(31, 41);
  contribute(CkCallback(CkReductionTarget(Initialize,Exit),srcProxy));
}

void PE::execute_seq() {
  while(getMinTime() < PE_VALUE(g_tw_ts_end)) {
    schedule_nextLP_no_save();
  }
  CkExit();
}

void PE::execute_cons() {
  while(getMinTime() < gvt + PE_VALUE(g_tw_lookahead)) {
    schedule_nextLP_no_save();
  }

  GVT_begin();
}

void PE::execute_opt() {
  if(++gvt_cnt > PE_VALUE(g_tw_gvt_interval)) {
    GVT_begin();
    gvt_cnt = 0;
    return;
  }
  process_cancel_q();

  for(int events = 0; events < PE_VALUE(g_tw_mblock); events++) {
    if(!schedule_nextLP())  break;
  }
  thisProxy[CkMyPe()].execute_opt();
}

void PE::process_cancel_q() {
  for(int pe_i = 0; pe_i < cancel_q.size(); pe_i++) {
    cancel_q[pe_i]->process_cancel_q();
  }
}

/* For now, in the synchronous version, invoke completion detection that leads
 * to a global reduction to find GVT - these may be merged later. The
 * target of the reduction should be the GVT_end function.
 */
void PE::GVT_begin() {
  if(!CkMyPe()) {
    /* TODO: Provide option for using completion detection */
    CkStartQD(CkCallback(CkIndex_PE::GVT_contribute(), thisProxy));
  }
}

void PE::GVT_contribute() {
  Time minTime = getMinTime();
  contribute(sizeof(Time), &minTime,CkReduction::min_double, CkCallback(CkReductionTarget(PE,GVT_end),thisProxy));
}

void PE::GVT_end(Time newGVT) {
  globals->lastGVT = gvt;
  gvt = newGVT;
  if(newGVT == DBL_MAX) {
    if(!CkMyPe()) {
      CkExit();
    }
  } else {
    if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
      thisProxy[CkMyPe()].execute_cons();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      collect_fossils();
      thisProxy[CkMyPe()].execute_opt();
    }
  }
}

/* Go over the oldest events queue and call fossil collection on the LPs with
 * entries less than equal to the new GVT. This may in turn lead to several updates
 * on this queue for LP's time stamps.
 */
void PE::collect_fossils() {
  LPToken *min = oldestEvents.top();
  while((min != NULL) && (min->ts < gvt)) {
    min->lp->fossil_me(gvt);
    min = oldestEvents.top();
  }
}

/* Using the nextEvents queue, execute events
 * on the LPs in chronological order. Eric noted
 * that we may be able to optimize here by passing
 * the next time to the LP which can continue
 * executing its events till the next time (instead of
 * only executing one event and returning the control).
 */
int PE::schedule_nextLP() {
  LPToken *min = nextEvents.top();
  if(min == NULL) return 0;
  /* TODO: this is not right, we want to pass the time stamp of the next event */
  currTime = min->ts;
  min->lp->execute_me(nextEvents.top()->ts);
  return 1;
}

int PE::schedule_nextLP_no_save() {
  LPToken *min = nextEvents.top();
  if(min == NULL) return 0;
  /* TODO: this is not right, we want to pass the time stamp of the next event */
  currTime = min->ts;
  min->lp->execute_me_no_save(nextEvents.top()->ts);
  return 1;
}
#include "pe.def.h"
