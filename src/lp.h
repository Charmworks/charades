#include "lp.decl.h"

#include "typedefs.h"
// TODO: Maybe put LP tokens in a different place
#include "pe_queue.h"

#include <vector>
#include <queue>

// The LPType contains function pointers for handling/reversing events as well
// as maps on how to locate LP structs based on their global ids.
struct LPType {
  event_f execute;
  revent_f reverse;
  map_f global_map;
  map_f local_map;
};

// TODO: Need to flesh this out more
// Right now, an LPStruct is an LPType, as well as its state.
struct LPStruct {
  LPType* type;
  void* state;
};

// TODO: This needs some work, especially since we don't know how we are dealing
// with globals such as type yet.
struct Event : public CMessage_Event {
  Time ts;
  unsigned local_dest;
};

typedef std::vector<LPStruct*> LPList;
typedef std::deque<Event*> ProcessedQueue;
typedef std::priority_queue<Event*> PriorityQueue;

class LP : public CBase_LP {
private:
  LPToken next_token;
  LPToken oldest_token;

	LPList lp_structs;
	PriorityQueue events;
	ProcessedQueue processed_events;
public:
	LP(); /**< constructor */

	void recv_event(Event*); /**< receive an event designated for me and add to the PE's and my event Q */

	void execute_me(Time); /**< execute the events with least time stamp till the given ts*/
	void rollback_me(Time); /**< rollback this collection of LPs until the given ts */
	void fossil_me(Time); /**< collect fossils till next the given gvt_ts */
};
