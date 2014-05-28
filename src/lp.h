#include "lp.decl.h"

#include "typedefs.h"
// TODO: Maybe put LP tokens in a different place
#include "pe_queue.h"

#include <vector>
#include <queue>

struct LPType {
  event_f execute;
  revent_f reverse;
  map_f map;
  map_local_f local_map;
};

struct LPData {
  LPType* type;
  // TODO: Haven't dealt with state yet
  void* state;
};

// TODO: Instead of type, maybe we could have the sender (who already knows
// the type) compute the local index and send that instead.
struct Event : public CMessage_Event {
  LPType* type;
  tw_stime ts;
  tw_lpid dest_id;
  tw_lpid source_id;
};

typedef std::vector<LPData*> LPList;
typedef std::deque<Event*> ProcessedQueue;
typedef std::priority_queue<Event*> PriorityQueue;

class LP : public CBase_LP {
private:
  LPToken token;
	LPList lpData;
	PriorityQueue events;
	ProcessedQueue processedEvents;

public:
	LP(); /**< constructor */

	void recv_event(Event*); /**< receive an event designated for me and add to the PE's and my event Q */

	void execute_me(Time); /**< execute the events with least time stamp till the given ts*/
	void rollback_me(Time); /**< rollback this collection of LPs until the given ts */
	void fossil_me(Time); /**< collect fossils till next the given gvt_ts */
};
