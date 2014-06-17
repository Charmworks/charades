#ifndef _EVENT_H
#define _EVENT_H

#include "typedefs.h"

#include "lp_chare.decl.h"

// TODO: This needs some work, especially since we don't know how we are dealing
// with globals such as type yet. It would also be nice if we could decouple an
// event struct from the Charm++ infrastructure.
struct Event : public CMessage_Event {
  Time ts;
  unsigned index;
  unsigned global_id;
  unsigned local_id;
};

void tw_event_send(tw_event * event);
tw_event* tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender);

#endif
