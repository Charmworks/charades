#include "pe.h"

void PE::GVT_begin() {
  /* For now, in the synchronous version,
   * invoke completion detection that also
   * performs global reduction to find GVT -
   * this merging need some thinking. The
   * target of this operation should be the
   * GVT_end function.
   */
}

void PE::update_nextEvents(LP_Entry) {
  /* Simple inline function to update the
   * time entry for a given LP in the
   * queue from which the PE picks the
   * next event to be executed. Need to
   * pin point a way to uniquely and
   * quickly identify the LP in the queue.
   */
}

void PE::update_oldestEvents(LP_Entry) {
  /* Simple inline function to update the
   * time entry for a given LP in the
   * queue which is used by the PE to
   * determine the LPs on which fossil
   * collection is needed.
   */
}

void PE::collect_fossils() {
  /* Go over the oldest events queue and
   * call fossil collection on the LPs with
   * entries less than equal to the new GVT.
   * This may in turn lead to several updates
   * on this queue for LP's time stamps.
   */
}

void PE::schedule_nextLP() {
  /* Using the nextEvents queue, execute events
   * on the LPs in chronological order. Eric noted
   * that we may be able to optimize here by passing
   * the next time to the LP which can continue
   * executing its events till the next time (instead of
   * only executing one event and returning the control).
   */
}
