#include "pe.decl.h"

class LP;

class PE: public CBase_PE {
private:
	PriorityQueue nextEvents; /**< queue to store the time stamp for next events that an LP has to execute*/
	PriorityQueue oldestEvents; /**< queue to store the time stamp for the earliest event that an LP has execute beyond the last computed GVT*/
public:
	PE(); /**< constructor */

	/** \brief Methods for GVT computation */
	void GVT_begin(); /**< begin gvt computation*/

	/** \brief Methods for updating current information */
	void update_next(LP*,tw_stime);//LP_Entry); /**< update the entry for a given LP in the nextEvents */
	void update_oldest(LP*,tw_stime);//LP_Entry); /** < update the entry for a given LP in the oldestEvents */

	void collect_fossils(); /**< collect fossils */
	void schedule_next(); /**< find the smallest time step and execute */
};
