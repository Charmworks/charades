#ifndef EVENT_H_
#define EVENT_H_

#include "event.decl.h"
#include "typedefs.h"

struct RemoteEvent : public CMessage_RemoteEvent {
  public:
    char *userData;

    // Used for the async completion detection algorithm
    unsigned phase;

    // These three fields make up the unique key for identifying events
    EventID event_id;
    Time ts;
    tw_peid send_pe;

    // The global id of the destination lp
    tw_lpid dest_lp;
};

#endif
