#include "pe.h"
#include "ross_util.h"
#include "ross_api.h"
#include "mpi-interoperate.h"
#include "float.h"
#include "charm_functions.h"

CProxy_PE pes;

Globals* get_globals() {
  return pes.ckLocalBranch()->globals;
}

Statistics* get_statistics() {
  return pes.ckLocalBranch()->statistics;
}

// TODO(eric): These should probably be moved to a more general Charm backend file
int tw_ismaster() {
  return (CkMyPe() == 0);
}

int tw_nnodes() {
  return CkNumPes();
}

void charm_exit() {
  CharmLibExit();
}

// Starts the simulation by calling the scheduler on all pes
void charm_run() {
  if(tw_ismaster()) DEBUG("[%d] Initializing schedulers \n", CkMyPe());
  if (tw_ismaster()) {
    PE_VALUE(total_time) = CkWallTimer();
    if(PE_VALUE(g_tw_synchronization_protocol) == SEQUENTIAL) {
      pes.execute_seq();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
      pes.execute_cons();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      pes.execute_opt();
    } else {
      tw_error(TW_LOC, "Incorrect scheduler values, Aborting\n");
    }
  }
  StartCharmScheduler();
}

#undef PE_VALUE
#define PE_VALUE(x) globals->x

PE::PE(CProxy_Initialize srcProxy) : gvt_cnt(0) {
  // init globals
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
  globals->g_tw_max_events_buffered = 1000;
  globals->g_tw_min_detected_offset = DBL_MAX;
  globals->g_tw_lookahead = .005;
  globals->g_init_map = init_block_map;
  globals->g_local_map = local_block_map;
  globals->lastGVT = 0.0;
  globals->netEvents = 0;
  globals->total_time = 0.0;
  gvt = 0.0;

  // init stats
  statistics = new Statistics;
  statistics->s_max_run_time = 0.0;
  statistics->s_net_events = 0;
  statistics->s_nevent_processed = 0;
  statistics->s_nevent_abort = 0;
  statistics->s_e_rbs = 0;
  statistics->s_rb_total = 0;
  statistics->s_rb_primary = 0;
  statistics->s_rb_secondary = 0;
  statistics->s_fc_attempts = 0;
  statistics->s_pq_qsize = 0;
  statistics->s_nsend_network = 0;
  statistics->s_nread_network = 0;
  statistics->s_nsend_remote_rb = 0;
  statistics->s_nsend_loc_remote = 0;
  statistics->s_nsend_net_remote = 0;
  statistics->s_ngvts = 0;
  statistics->s_mem_buffers_used = 0;
  statistics->s_pe_event_ties = 0;
  statistics->s_min_detected_offset = 0.0;
  statistics->s_total = 0;
  statistics->s_net_read = 0;
  statistics->s_gvt = 0;
  statistics->s_fossil_collect = 0;
  statistics->s_event_abort = 0;
  statistics->s_event_process = 0;
  statistics->s_pq = 0;
  statistics->s_rollback = 0;
  statistics->s_cancel_q = 0;
  statistics->s_avl = 0;

  cancel_q.resize(0);
  thisProxy[CkMyPe()].initialize_rand(srcProxy);
}

void PE::initialize_rand(CProxy_Initialize srcProxy) {
  rng = tw_rand_init(31, 41);
  contribute(CkCallback(CkReductionTarget(Initialize,Exit),srcProxy));
}

void PE::execute_seq() {
  while(getMinTime() < PE_VALUE(g_tw_ts_end)) {
    PE_STATS(s_nevent_processed)+= schedule_next_LP();
  }
  CkExit();
}

void PE::execute_cons() {
  while(getMinTime() < gvt + PE_VALUE(g_tw_lookahead)) {
    schedule_next_LP();
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
    if(!schedule_next_LP())  break;
  }
  thisProxy[CkMyPe()].execute_opt();
}

void PE::process_cancel_q() {
  for(int pe_i = 0; pe_i < cancel_q.size(); pe_i++) {
    cancel_q[pe_i]->process_cancel_q();
  }
}

Time PE::getMinTime() {
  Time min;

  if(nextEvents.top() != NULL) {
    min = nextEvents.top()->ts;
  } else {
    min = DBL_MAX;
  }

  // TODO: This could probably be optimized
  if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
    for(int pe_i = 0; pe_i < cancel_q.size(); pe_i++) {
      Time newTime = cancel_q[pe_i]->getMinCancelTime();
      if(newTime < min) {
        min = newTime;
      }
    }
  }

  return min;
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
  DEBUG4("[%d] ***************GVT begins*******************\n",CkMyPe());
}

void PE::GVT_contribute() {
  Time minTime = getMinTime();
  DEBUG4("[%d] ***************GVT contribute %lf******************\n",CkMyPe(), minTime);
  contribute(sizeof(Time), &minTime,CkReduction::min_double, CkCallback(CkReductionTarget(PE,GVT_end),thisProxy));
}

void PE::GVT_end(Time newGVT) {
  if(tw_ismaster()) DEBUG4("[%d] GVT computed %lf\n",CkMyPe(), newGVT);
  globals->lastGVT = gvt;
  gvt = newGVT;
  if(newGVT == DBL_MAX) {
    PE_VALUE(total_time) = CkWallTimer() - PE_VALUE(total_time);
    contribute(sizeof(double), &(globals->netEvents), CkReduction::sum_double, CkCallback(CkReductionTarget(PE,endExec),thisProxy[0]));
  } else {
    if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
      thisProxy[CkMyPe()].execute_cons();
    } else if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      collect_fossils();
      thisProxy[CkMyPe()].execute_opt();
    }
  }
}

void PE::endExec(double totalNetEvents) {
  CkPrintf("Total events executed: %.0lf\n", totalNetEvents);
  CkPrintf("Total time: %f s\n", PE_VALUE(total_time));
  CkPrintf("Event rate: %f events/s\n", totalNetEvents/PE_VALUE(total_time));
  CkExit();
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
int PE::schedule_next_LP() {
  LPToken *min = nextEvents.top();
  if(min == NULL) return 0;
  if (min->ts == currTime) {
    PE_STATS(s_pe_event_ties)++;
  }
  // TODO: this is not right, we want to pass the time stamp of the next event
  currTime = min->ts;
  min->lp->execute_me(nextEvents.top()->ts);
  return 1;
}

#include "pe.def.h"
