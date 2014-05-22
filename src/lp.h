#include "lp.decl.h"

#include "typedefs.h"

#include <vector>
#include <queue>

typedef std::vector<void*> List;
typedef std::queue<Event*> FIFO;
typedef std::priority_queue<Event*> PriorityQueue;

struct Event : public CMessage_Event {
  Time ts;
};

class LP : public CBase_LP {
	LP(); /**< constructor */

	List userEntities;
	PriorityQueue events;
	FIFO processedEvents;

	void recv_event(Event*); /**< receive an event designated for me and add to the PE's and my event Q */

	void execute_me(Time); /**< execute the events with least time stamp till the given ts*/
	void fossil_me(Time); /**< collect fossils till next the given gvt_ts */
};
