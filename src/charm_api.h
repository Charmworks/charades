// Defines a C-Style API that is exposed to the ross backend

#ifndef _CHARM_API_
#define _CHARM_API_
// PE API

// LP API
void create_lps();
void init_lps();
void set_current_event(tw_lp*, tw_event*);
tw_event* current_event(tw_lp*);
//tw_stime tw_now(tw_lp*);

// Event API
tw_event * charm_allocate_event(int needMsg = 1);
void charm_free_event(tw_event * e);
void charm_event_cancel(tw_event * e);
int charm_event_send(unsigned, tw_event * e);
void charm_anti_send(unsigned, tw_event * e);

#endif
