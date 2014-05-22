#include "lp.h"
#include "pe.h"

extern CProxy_PE pes;

LP::LP() {}

void LP::recv_event(Event* e) {
  if (e->ts < events.top()->ts) {
    pes.ckLocalBranch()->update_next(this, e->ts);
  }
  events.push(e);
}

void LP::execute_me(Time ts) {
  while (events.top()->ts <= ts) {
    Event* e = events.top();
    events.pop();
    // Execute e on the associated LP
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
