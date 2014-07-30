#ifndef _LP_CHARE_H
#define _LP_CHARE_H

#include "lp_chare.decl.h"

#include "lp_structs.h"
#include "typedefs.h"
#include "processed_queue.h"

#include <vector>
#include <queue>

class Event;

// Tokens owned by LP chares that are used by the PE queues that control
// scheduling and fossil collection. Each token has a direct pointer to its LP,
// the timestamp associated with the token, and the index of its location in the
// queue.
struct LPToken {
  private:
    LPChare* lp;
    Time ts;
    unsigned index;

  public:
    LPToken(LPChare* lp) : lp(lp) {}
    friend class PEQueue;
    friend class PE;
};

typedef std::vector<LPStruct> LPList;
typedef std::priority_queue<Event*> PriorityQueue;

class LPChare : public CBase_LPChare {
  private:
    LPToken next_token;
    LPToken oldest_token;

    LPList lp_structs;
    PriorityQueue events;
    ProcessedQueue processed_events;

    // TODO: Maybe it would be better to just poll the top of the events queue instead of maintaining this
    Time current_time;
  public:
    LPChare(); /**< constructor */

    void recv_event(Event*); /**< receive an event designated for me and add to the PE's and my event Q */

    void execute_me(Time); /**< execute the events with least time stamp till the given ts*/
    void rollback_me(Time); /**< rollback this collection of LPs until the given ts */
    void fossil_me(Time); /**< collect fossils till next the given gvt_ts */

    Time now() const { return current_time; }
};
#endif
