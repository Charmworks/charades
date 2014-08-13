#include "lp.h"
#include "pe.h"
#include "event.h"
#include "globals.h"

#include "avl_tree.h"

// Readonly variables for the global proxies.
extern CProxy_PE pes;
extern CProxy_LP lps;

#ifndef NO_FORWARD_DECLS
void tw_error(const char* file, int line, const char* fmt, ...);
#endif

// TODO: Temporary mapping definitions. Once mapping is nailed down, this
// should be moved.
typedef tw_lpid(*local_to_global_map) (unsigned, tw_lpid);
extern local_to_global_map ltog;

// This is the API which allows the ROSS code to initialize the Charm backend.
void create_lps() {
  lps = CProxy_LP::ckNew(PE_VALUE(g_num_lp_chares));
}

// Create LPStructs based on mappings, and do initial registration with the PE.
// TODO: We may just want to pass in a mapping as a param to the constructor.
LP::LP() : next_token(this), oldest_token(this),
    lp_structs(PE_VALUE(g_lps_per_chare)), uniqID(0), enqueued_cancel_q(false) {
  pes.ckLocalBranch()->register_lp(&next_token, 0.0, &oldest_token, 0.0);

  // Create array of LPStructs based on globals
  // TODO: Should the init function be called here as well?
  unsigned offset = thisIndex * PE_VALUE(g_lps_per_chare);
  for (int i = 0; i < PE_VALUE(g_lps_per_chare); i++) {
    lp_structs[i].owner = this;
    lp_structs[i].gid = ltog(thisIndex, i);
    lp_structs[i].state = NULL;
    // TODO: Nail down mappings.
    //lp_structs[i].type = global_type_map(lp_structs[i].gid);
  }

  // TODO: Init LP RNG streams
}

// Entry method for sending events to LPs.
// 1) Check if the event is earlier than our earliest and update the PE.
// 2) Check to see if we need a rollback.
// 3) Push event into the priority queue.
void LP::recv_event(RemoteEvent* event) {
  // TODO: Difference between tw_event_new and allocate event?
  Event *e = allocateEvent(0);
  e->event_id = event->event_id;
  e->ts = event->ts;
  e->dest_lp = event->dest_lp;
  // TODO: Figure out pe
  //e->send_pe = event->send_pe;

  if(event->isAnti) {
    // TODO: What is all_events
    Event *real_e = avlDelete(&all_events, e);
    delete e;
    e = real_e;
    event_cancel(e);
    delete event;
  } else {
    avlInsert(&all_events, e);
    e->userData = event->userData;
    e->eventMsg = event;
    /* TODO Somehow get lop LP pointer */
    //e->dest_lp =
    if (e->ts < events.top()->ts) {
      pes.ckLocalBranch()->update_next(&next_token, e->ts);
    }
    if (e->ts < processed_events.back()->ts) {
      rollback_me(e->ts);
    }
    events.push(e);
    e->state.owner = TW_chare_q;
  }
}

// Execute events up to timestamp ts.
// 1) If next event is still earlier than ts, pop it.
// 2) Execute the popped event on its destination LP.
// 3) Update the PE with our new earliest timestamp.
void LP::execute_me(tw_stime ts) {
  while (events.top()->ts <= ts) {
    Event* e = events.top();
    events.pop();
    current_time = e->ts;
    // TODO: Instead of local id, use a direct pointer to the LP
    //LPStruct *lp = &lp_structs[e->local_id];
    //lp->type->execute(lp, e);
    currEvent = e;
    processed_events.push_front(e);
    e->state.owner = TW_rollback_q;
  }
  pes.ckLocalBranch()->update_next(&next_token, events.top()->ts);
}

// Fossil collect all events older than the passed in GVT.
// 1) If the next event is older than the current gvt pop it and delete it.
// 2) Update the PE with our oldest unprocessed event time.
void LP::fossil_me(tw_stime gvt) {
  while (processed_events.back()->ts <= gvt) {
    Event* e = processed_events.back();
    processed_events.pop_back();
    delete e;
  }
  pes.ckLocalBranch()->update_oldest(&oldest_token, processed_events.back()->ts);
}

// Rollback all processed events up to the passed in timestamp.
// 1) If the most recent processed event is older than the timestamp, pop it.
// 2) Execute the reverse handler on the target lp and popped event.
// 3) Push the popped event onto the event priority queue.
// Note: This method is not responsible for updating the PE.
void LP::rollback_me(tw_stime ts) {
  while (processed_events.back()->ts > ts) {
    Event* e = processed_events.front();
    processed_events.pop_front();
    tw_event_rollback(e);
    current_time = processed_events.front()->ts;
    events.push(e);
  }
}

void LP::rollback_me(Event *event) {
  Event* e = processed_events.front();
  processed_events.pop_front();

  while(e != event)
  {
    tw_event_rollback(e);
    events.push(e);
    e = processed_events.front();
    processed_events.pop_front();
  }

  tw_event_rollback(e);
  current_time = processed_events.front()->ts;
}

void LP::process_cancel_q() {
  tw_event    *cev, *nev;

  while (cancel_q) {
    cev = cancel_q;
    cancel_q = NULL;

    for (; cev; cev = nev) {
      nev = cev->cancel_next;

      switch (cev->state.owner) {
        case TW_anti_msg:
          tw_event_free(this, cev);
          break;

        case TW_chare_q:
          delete_pending(cev);
          tw_event_free(this, cev);
          break;

        case TW_rollback_q:
          rollback_me(cev);
          tw_event_free(this, cev);
          break;

        default:
          tw_error(TW_LOC, "Event in cancel_q, but owner %d not recognized", cev->state.owner);
      }
    }
  }
}
