#include "lp.decl.h"

class LP : public CBase_LP {
	LP(); /**< constructor */

	List userEntities;
	PriorityQueue events;
	FIFO processedEvents;

	void recv_event(EventType); /**< receive an event designated for me and add to the PE's and my event Q */

	void execute_me(Timestep ts); /**< execute the events with least time stamp till the given ts*/
	void fossil_me(Timestep gvt_ts); /**< collect fossils till next the given gvt_ts */
};