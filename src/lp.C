#include "lp.h"
#include "pe.h"

extern CProxy_PE pes;

LP::LP() {}

void LP::recv_event(Event* e) {
  if (e->ts < events.top()->ts) {
    pes.ckLocalBranch()->update_next(this, e->ts);
  }
  // Do a check on the back of the processed queue and maybe rollback
  // Also may need to cancel things in the case of an anti-message
  events.push(e);
}

void LP::execute_me(Time ts) {
  while (events.top()->ts <= ts) {
    Event* e = events.top();
    events.pop();
    userEntities[map(e->dest_id)]->execute(e);
    processedEvents.push(e);
  }
  pes.ckLocalBranch()->update_next(this, events.top()->ts);
}

void LP::fossil_me(Time gvt) {
  while (processedEvents.front()->ts <= gvt) {
    Event* e = processedEvents.front();
    processedEvents.pop();
    delete e;
  }
  pes.ckLocalBranch()->update_oldest(this, processedEvents.front()->ts);
}
