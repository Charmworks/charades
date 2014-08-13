#include "pe.h"

// This is the API which allows the ROSS code to initialize and access the
// Charm++ backend.

Globals* get_globals() {
  return pes.ckLocalBranch()->globals;
}

PE::PE(CProxy_Initialize srcProxy) : batchSize(20), gvt_cnt(0), gvt_freq(10) {
  contribute(CkCallback(CkReductionTarget(Initialize,Exit),srcProxy));
}

void PE::execute_seq() {
  while(getMinTime() < endTime)
    schedule_nextLP();
  }
}

void PE::execute_cons() {
  while(getMinTime() < gvt + lookahead) {
    schedule_nextLP();
  }

  GVT_begin();
}

void PE::execute_opt() {
  if(++gvt_cnt > gvt_freq) {
    GVT_begin();
    gvt_cnt = 0;
    return;
  }
  process_cancel_q();

  for(int events = 0; events < batchSize(); events++) {
    if(!schedule_nextLP())  break;
  }
  thisProxy[CkMyPe()].execute_opt();
}

void PE::process_cancel_q() {
  for(int pe_i = 0; pe_i < cancel_q.size(); pe_i++) {
    cancel_q[pe_i].process_cancel_q();
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
  gvt = newGVT;
  collect_fossils();
  thisProxy[CkMyPe()].execute();
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
  min->lp->execute_me(nextEvents.top()->ts);
  currTime = min->ts;
  return 1;
}
#include "pe.def.h"
