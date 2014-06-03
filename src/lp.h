#include "lp.decl.h"

#include "typedefs.h"
#include "pe_queue.h"

#include <vector>
#include <queue>

class Event;

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
};

// The LPType contains function pointers for handling/reversing events as well
// as maps on how to locate LP structs based on their global ids.
struct LPType {
  init_f init;
  event_f execute;
  revent_f reverse;
  finalize_f finalize;
  map_f global_map;
  map_f local_map;
};

// TODO: Need to flesh this out more
// Right now, an LPStruct is an LPType, as well as its state.
struct LPStruct {
  LP* owner;
  unsigned gid;
  void* state;
  LPType* type;
};

typedef std::vector<LPStruct> LPList;
typedef std::deque<Event*> ProcessedQueue;
typedef std::priority_queue<Event*> PriorityQueue;

class LP : public CBase_LP {
  private:
    LPToken next_token;
    LPToken oldest_token;

    LPList lp_structs;
    PriorityQueue events;
    ProcessedQueue processed_events;

    // TODO: Maybe it would be better to just poll the top of the events queue instead of maintaining this
    Time current_time;
  public:
    LP(); /**< constructor */

    void recv_event(Event*); /**< receive an event designated for me and add to the PE's and my event Q */

    void execute_me(Time); /**< execute the events with least time stamp till the given ts*/
    void rollback_me(Time); /**< rollback this collection of LPs until the given ts */
    void fossil_me(Time); /**< collect fossils till next the given gvt_ts */

    Time now() const { return current_time; }
};
