#ifndef EVENT_H_
#define EVENT_H_

#include "event.decl.h"
#include "typedefs.h"

struct RemoteEvent : public CMessage_RemoteEvent {
  public:
  char *userData;
  EventID event_id;
  Time ts;
  tw_lpid dest_lp;
  tw_peid send_pe;
  bool isAnti;

  RemoteEvent() : isAnti(false) { }
};

#endif
