#ifndef _LP_H
#define _LP_H

#include "lp.decl.h"

#include "typedefs.h"
#include "lp_struct.h"
#include "pending_queue.h"
#include "processed_queue.h"

#include <vector>

class RemoteEvent;

// Tokens owned by LP chares that are used by the PE queues that control
// scheduling and fossil collection. Each token has a direct pointer to its LP,
// the timestamp associated with the token, and the index of its location in the
// queue.
struct LPToken {
  private:
    LP* lp;
    Time ts;
    unsigned index;

  public:
    LPToken(LP* lp) : lp(lp) {}
    friend class PEQueue;
    friend class PE;
};

typedef std::vector<LPStruct> LPList;

class LP : public CBase_LP {
  private:
    LPToken next_token;
    LPToken oldest_token;

    LPList lp_structs;
    PendingQueue events;
    ProcessedQueue processed_events;
    AvlTree all_events;

    // TODO: Maybe it would be better to just poll the top of the events queue instead of maintaining this
    Time current_time;
  public:
    Event *cancel_q;
    Event *currEvent;
    EventID uniqID;
    bool enqueued_cancel_q;
    LP(); /**< constructor */

    void recv_event(RemoteEvent*); /**< receive an event designated for me and add to the PE's and my event Q */

    void execute_me(Time); /**< execute the events with least time stamp till the given ts*/
    void rollback_me(Time); /**< rollback this collection of LPs until the given ts */
    void rollback_me(Event*); /**< rollback this collection of LPs until the given ts */
    void fossil_me(Time); /**< collect fossils till next the given gvt_ts */
    void process_cancel_q();

    void delete_pending(Event *e);

    Time now() const { return current_time; }
};
#endif
