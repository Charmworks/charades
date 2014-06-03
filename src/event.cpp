#include "event.h"
#include "lp.h"

extern CProxy_LP lps;

void tw_send_event(tw_event* e) {
  lps(e->index).recv_event(e);
}

tw_event* tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender) {
  tw_event* e = new tw_event();
  e->ts = sender->owner->now() + offset_ts;
  e->index = sender->type->global_map(dest_gid); 
  e->global_id = dest_gid;
  e->local_id = sender->type->local_map(dest_gid);
}
