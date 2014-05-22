#include "lp.decl.h"

#include "typedefs.h"

#include <vector>
#include <queue>

struct user_entity {
  event_f execute;
};

typedef std::vector<user_entity*> List;
typedef std::queue<Event*> FIFO;
typedef std::priority_queue<Event*> PriorityQueue;

struct Event : public CMessage_Event {
  Time ts;
  tw_lpid dest_id;
  tw_lpid source_id;
};

class LP : public CBase_LP {
private:
	List userEntities;
	PriorityQueue events;
	FIFO processedEvents;

  map_local_f map;

public:
	LP(); /**< constructor */

	void recv_event(Event*); /**< receive an event designated for me and add to the PE's and my event Q */

	void execute_me(Time); /**< execute the events with least time stamp till the given ts*/
	void fossil_me(Time); /**< collect fossils till next the given gvt_ts */
};
