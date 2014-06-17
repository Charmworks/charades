#include "pe.h"

void PE::execute() {
  if(++gvt_cnt > gvt_freq) {
    GVT_begin();
    gvt_cnt = 0;
    return;
  }
  for(int events = 0; events < batchSize(); events++) {
    if(!schedule_nextLP())  break;
  }
  thisProxy[CkMyPe()].execute();
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
  nextEvents.remove(top);
  min->lp->execute_me(nextEvents.top()->ts);
  return 1;
}
