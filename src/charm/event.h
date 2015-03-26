#ifndef EVENT_H_
#define EVENT_H_

#include "event.decl.h"
#include "typedefs.h"

struct RemoteEvent : public CMessage_RemoteEvent {
  public:
    RemoteEvent() {
      clear();
    }
    void clear() {
      event_id = 0;
      ts = 0.0;
      send_pe = 0;
      dest_lp = 0;
    }

    char *userData;

    // These three fields make up the unique key for identifying events
    EventID event_id;
    Time ts;
    tw_peid send_pe;

    // The global id of the destination lp
    tw_lpid dest_lp;

    virtual void pup(PUP::er& p);
};

class Event;
void pup_pending_event(PUP::er& p, Event* e);
void pup_processed_event(PUP::er& p, Event* e);
void pup_sent_event(PUP::er& p, Event* e);

#endif
