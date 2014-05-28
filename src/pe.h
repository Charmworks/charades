#include "pe.decl.h"

#include "pe_queue.h"

class LP;

class PE: public CBase_PE {
private:
	PEQueue nextEvents; /**< queue to store the time stamp for next events that an LP has to execute*/
	PEQueue oldestEvents; /**< queue to store the time stamp for the earliest event that an LP has execute beyond the last computed GVT*/
public:
	PE(); /**< constructor */

	/** \brief Methods for GVT computation */
	void GVT_begin(); /**< begin gvt computation*/

  void register_lp(LPToken*, Time, LPToken*, Time);
  void unregister_lp(LPToken*, Time, LPToken*, Time);

	/** \brief Methods for updating current information */
	void update_next(LPToken*, Time); /**< update the entry for a given LP in the nextEvents */
	void update_oldest(LPToken*, Time); /** < update the entry for a given LP in the oldestEvents */

	void collect_fossils(); /**< collect fossils */
	void schedule_next(); /**< find the smallest time step and execute */
};
