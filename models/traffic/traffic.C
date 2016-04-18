#include "traffic.h"


//Map consists of NUM_CELLS_X x NUM_CELL_Y LPS


tw_lpid Cell_ComputeMove( tw_lpid lpid, int direction )
{
	tw_lpid lpid_x, lpid_y;
	tw_lpid n_x, n_y;
	tw_lpid dest_lpid;

	lpid_y = lpid / NUM_CELLS_X;
	lpid_x = lpid - (lpid_y * NUM_CELLS_X);

	switch( direction )
	{

	case 0:
		//East
		n_x = (lpid_x + 1) % NUM_CELLS_X;
		n_y = lpid_y;
		break;

	case 1:
		//North
		n_x = lpid_x;
		n_y = ((lpid_y - 1) + NUM_CELLS_Y) % NUM_CELLS_Y;
		break;

	case 2:
		//West
		n_x = ((lpid_x - 1) + NUM_CELLS_X) % NUM_CELLS_X;
		n_y = lpid_y;
		break;


	case 3:
		//South
		n_x = lpid_x;
		n_y = (lpid_y + 1) % NUM_CELLS_Y;
		break;


	default:
		tw_error( TW_LOC, "Bad direction value \n");
	}

	dest_lpid = (tw_lpid) (n_x + (n_y * NUM_CELLS_X));
	return( dest_lpid );
}
unsigned uniform_start(tw_lpid lpid)
{
	return g_traffic_start_events;

}

unsigned non_uniform_start(tw_lpid lpid)
{
	tw_lpid lp_x = lpid % NUM_CELLS_X; //lpid -> (lp_x,lp_y)
	tw_lpid lp_y = lpid / NUM_CELLS_X;
	if( (lp_x >= g_startX) && (lp_x < (g_startX + g_startSize)) && (lp_y >= g_startY) && (lp_y < (g_startY + g_startSize)))
		return g_percentStart * g_traffic_start_events*INTERSECTION_LPS / (g_startSize * g_startSize);
	else return  (1-g_percentStart) * g_traffic_start_events*INTERSECTION_LPS / (INTERSECTION_LPS -  (g_startSize * g_startSize));

}


//CHARE MAP
unsigned  traffic_chare_map( tw_lpid lpid)
{
	tw_lpid lp_x = lpid % NUM_CELLS_X; //lpid -> (lp_x,lp_y)
	tw_lpid lp_y = lpid / NUM_CELLS_X;
	tw_lpid vp_index_x = lp_x / g_cells_per_vp_x;
	tw_lpid vp_index_y = lp_y / g_cells_per_vp_y;
	tw_lpid vp_index = vp_index_x + (vp_index_y * (NUM_VP_X));
	
	return (unsigned) vp_index;
}

// LOCAL MAP

tw_lpid traffic_local_map( tw_lpid lpid)
{

	tw_lpid lp_x = lpid % NUM_CELLS_X; //lpid -> (lp_x,lp_y)
	tw_lpid lp_y = lpid / NUM_CELLS_X;
	tw_lpid vp_index_x = lp_x % g_cells_per_vp_x;
	tw_lpid vp_index_y = lp_y % g_cells_per_vp_y;
	tw_lpid vp_index = vp_index_x + (vp_index_y * (g_cells_per_vp_x));
	
	return vp_index;
}

tw_lptype mylps[] = {
	{
            (init_f) Intersection_StartUp,
			(event_f) Intersection_EventHandler,
			(revent_f) Intersection_RC_EventHandler,
			(final_f) Intersection_Statistics_CollectStats,
			sizeof(Intersection_State)
	},
	{ 0 },
};
tw_lptype * traffic_type_map(tw_lpid global_id)
{
	return &mylps[0];
}

tw_lpid traffic_init_map(unsigned chare, tw_lpid local_id)
{
        int chare_x = chare % NUM_VP_X;
        int chare_y = chare / NUM_VP_X;

        int off_x = chare_x * g_cells_per_vp_x;
        int off_y = chare_y * g_cells_per_vp_y;

        off_x += (local_id % g_cells_per_vp_x);
        off_y += (local_id / g_cells_per_vp_x);

        return (tw_lpid) (off_y * NUM_CELLS_X + off_x);
}

const tw_optdef app_opt[] =
{
  TWOPT_GROUP("TRAFFIC Model"),
  TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
  TWOPT_STIME("percentStart",g_percentStart, "Pecent of cars that start in clustered block [0-1]"),
  TWOPT_UINT("startSize",g_startSize, "Size of start cluster block. X by X block"),
  TWOPT_UINT("startX",g_startX, "X coord of upper left corner of start cluster"),
  TWOPT_UINT("startY",g_startY, "Y coord of upper left corner of start cluster"),
   TWOPT_STIME("percentEnd",g_percentEnd, "Pecent of cars that end in clustered block [0-1]"),
  TWOPT_UINT("destSize",g_endSize, "Size of end cluster block. X by X block"),
  TWOPT_UINT("destX",g_endX, "X coord of upper left corner of end cluster"),
  TWOPT_UINT("destY",g_endY, "Y coord of upper left corner of end cluster"),
   TWOPT_UINT("start-events", g_traffic_start_events, "average start events per LP. start-events * numLPS = total start events"),
  TWOPT_UINT("carsPerRoad", MAX_CARS_ON_ROAD, "Max number of cars on a given road"),
  TWOPT_CHAR("run", run_id, "user supplied run name"),
  TWOPT_END()
};






int main(int argc, char * argv[]) 
{
	g_tw_ts_end = 30;
	g_tw_gvt_interval = 16;
	g_num_chares = 10;


	tw_opt_add(app_opt);
	tw_init(&argc, &argv);

	if( g_tw_lookahead > 1.0 )
		tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");

	INTERSECTION_LPS = g_total_lps;
	g_cells_per_vp = g_lps_per_chare;	
	NUM_CELLS_X = (unsigned) sqrt(INTERSECTION_LPS);
	NUM_CELLS_Y = NUM_CELLS_X;
	NUM_VP_X = (unsigned) sqrt( NUM_CELLS_X * NUM_CELLS_X / g_cells_per_vp);
	NUM_VP_Y = NUM_VP_X;
	g_cells_per_vp_x = NUM_CELLS_X/NUM_VP_X;
	g_cells_per_vp_y = NUM_CELLS_Y/NUM_VP_Y;	
	g_num_chares = NUM_VP_X * NUM_VP_Y;

	//reset mean based on lookahead
	mean = mean - g_tw_lookahead;
	if(g_percentStart > 0)
		lp_source_map = &non_uniform_start;
	else
		lp_source_map = &uniform_start;
	g_type_map = traffic_type_map;	
	g_chare_map = traffic_chare_map;	
	g_local_map = traffic_local_map;
	g_init_map = traffic_init_map;

	tw_define_lps(sizeof(Msg_Data), 0);
	if(tw_ismaster() )
	{
		printf("========================================\n");
		printf("Traffice Model Configuration..............\n");
		printf("   Lookahead..............%lf\n", g_tw_lookahead);
		printf("   Start-events...........%u\n", g_traffic_start_events);
		printf("   Mean...................%lf\n", mean);
		printf("   percentStart...........%lf\n", g_percentStart);
		printf("   startSize..............%u\n", g_startSize);
		printf("   startX.................%u\n", g_startX);
		printf("   startY.................%u\n", g_startY);
		printf("   percentEnd.............%lf\n", g_percentEnd);
		printf("   destSize..............%u\n", g_endSize);
		printf("   destX.................%u\n", g_endX);
		printf("   destY.................%u\n", g_endY);
		printf("   Max Cars on Road.......%u\n", MAX_CARS_ON_ROAD);
		printf("========================================\n\n");

	}
	tw_run();


//	printf("Number of Arivals: %lld\n", totalCars);
//	printf("Number of Cars reached their dest: %lld\n", carsFinished);
	tw_end();
	return 0;
}

void  Intersection_StartUp(Intersection_State *SV, tw_lp * lp) {
	tw_event* e;
	Msg_Data* msg;
	tw_stime ts;
	tw_lpid source, dest;
	tw_lpid sourceX, sourceY;
	tw_lpid destX, destY;

	// Initialize state variables
	SV->total_cars_arrived = 0;
	SV->total_cars_finished = 0;

	for (int i = 0; i<12;i++)
	{
		SV->inLane[i]=0;
		SV->outLane[i]=0;
	}

		// Create and send an event for each car with its own source and dest
	for (int i = 0; i < lp_source_map(lp->gid); i++) {
		// Compute the source LP for each car.
	
		// Create an arrival event foor whatever source LP we computed.
		ts = g_tw_lookahead + tw_rand_exponential(lp->rng, mean);
		e = tw_event_new(lp->gid, ts, lp);
		msg = (Msg_Data *)tw_event_data(e);
		msg->event_type = ARRIVAL;
		msg->car.current_lane = (tw_rand_integer(lp->rng,0,11));
		msg->car.in_out = IN;

		// Choose a destination for the car based on the distribution configuration.
		if (g_percentEnd > tw_rand_unif(lp->rng)) {
			destX = tw_rand_integer(lp->rng,0,g_endSize-1) + g_endX;
			destY = tw_rand_integer(lp->rng,0,g_endSize-1) + g_endY;
			dest = destX + NUM_CELLS_X * destY;
		} else {
			dest = tw_rand_integer(lp->rng,0,INTERSECTION_LPS-1);
			destX = dest % NUM_CELLS_X;
			destY = dest / NUM_CELLS_X;
		}

		// Set the x_to_go and y_to_go based on source and dest, and send the event
		msg->car.xDest = destX;
		msg->car.yDest = destY;
		tw_event_send(e);
	}
}

void Intersection_EventHandler(Intersection_State *SV, tw_bf *CV, Msg_Data *M, tw_lp *lp) 
{
	int new_event_direction=0;
	*(int *)CV = (int)0;

	// Create the event ahead of time so we can set its fields based on what kind
	// of event we are handling currently.
	tw_stime ts = g_tw_lookahead + tw_rand_exponential(lp->rng, mean);
	tw_event *CurEvent = tw_event_new(lp->gid, ts, lp);
	Msg_Data *NewM = (Msg_Data *)tw_event_data(CurEvent);
	NewM->car.xDest = M->car.xDest;
	NewM->car.yDest = M->car.yDest;
	NewM->car.current_lane = M->car.current_lane;
	NewM->car.in_out = M->car.in_out;
	int curY = lp->gid / NUM_CELLS_X;
	int curX = lp->gid - (curY * NUM_CELLS_X);



	switch (M->event_type) {

	case ARRIVAL:
	{ 
		if (curY==M->car.yDest && curX==M->car.xDest)
		{
			SV->total_cars_finished++;
			break;
		}
		SV->total_cars_arrived++;
		int newLane = (M->car.current_lane + 6) % 12;	//These lanes are offset by 6.
		SV->inLane[newLane]++;
		NewM->car.current_lane = newLane;
		// Schedule a departure in the future

		NewM->car.in_out = IN;
		NewM->event_type = DIRECTION_SELECT;
		tw_event_send(CurEvent);
		break;
	}
	case DEPARTURE:
	{
	 	SV->outLane[M->car.current_lane]--;
		new_event_direction = M->car.current_lane / 3;		
		CurEvent->dest_lp = Cell_ComputeMove(lp->gid, new_event_direction);
		NewM->event_type = ARRIVAL;
		tw_event_send(CurEvent);
		break;
	}
	case DIRECTION_SELECT:
	{
		int sent_back = 0;
		switch(M->car.current_lane % 4){

		case 0:		//Lanes with this value are all going South, ending up in lane 9,10,or 11
			if(M->car.xDest < curX && SV->outLane[11]  < MAX_CARS_ON_ROAD){
				SV->outLane[11]++;
				NewM->car.current_lane = 11;
			}
			else if(M->car.xDest > curX && SV->outLane[9] < MAX_CARS_ON_ROAD){
				SV->outLane[9]++;
				NewM->car.current_lane = 9;
			}
			else if(SV->outLane[10] < MAX_CARS_ON_ROAD){
				SV->outLane[10]++;
				NewM->car.current_lane = 10;
			}

			else{			
				sent_back =1;	
			}
			break;

		case 1:		//Lanes with this value are all going West, ending up in lane 6,7,or 8
			if(M->car.yDest < curY && SV->outLane[8]  < MAX_CARS_ON_ROAD){
				SV->outLane[8]++;
				NewM->car.current_lane = 8;
			}
			else if(M->car.yDest > curY && SV->outLane[6] < MAX_CARS_ON_ROAD){
				SV->outLane[6]++;
				NewM->car.current_lane = 6;
			}
			else if(SV->outLane[7] < MAX_CARS_ON_ROAD){
				SV->outLane[7]++;
				NewM->car.current_lane = 7;
			}

			else{			
				sent_back =1;	
			}
			break;

		case 2:		//Lanes with this value are all going North, ending up in lane 3,4,or 5
			if(M->car.xDest < curX && SV->outLane[3]  < MAX_CARS_ON_ROAD){
				SV->outLane[3]++;
				NewM->car.current_lane = 3;
			}
			else if(M->car.xDest > curX && SV->outLane[5] < MAX_CARS_ON_ROAD){
				SV->outLane[5]++;
				NewM->car.current_lane = 5;
			}
			else if(SV->outLane[4] < MAX_CARS_ON_ROAD){
				SV->outLane[4]++;
				NewM->car.current_lane = 4;
			}

			else{			
				sent_back =1;	
			}
			break;

		case 3:		//Lanes with this value are all going East, ending up in lane 0,1,or 2
			if(M->car.yDest < curY && SV->outLane[0]  < MAX_CARS_ON_ROAD){
				SV->outLane[0]++;
				NewM->car.current_lane = 0;
			}
			else if(M->car.yDest > curY && SV->outLane[2] < MAX_CARS_ON_ROAD){
				SV->outLane[2]++;
				NewM->car.current_lane = 2;
			}
			else if(SV->outLane[1] < MAX_CARS_ON_ROAD){
				SV->outLane[1]++;
				NewM->car.current_lane = 1;
			}

			else{			
				sent_back =1;	
			}
			break;
		}
		


		if(sent_back)
		{
			NewM->car.in_out = IN;
			NewM->event_type = DIRECTION_SELECT;
		}
		else{
			SV->inLane[M->car.current_lane]--;
			NewM->car.in_out = OUT;
			NewM->event_type = DEPARTURE;
		}
		tw_event_send(CurEvent);
		break;
	}
	}
}


void Intersection_RC_EventHandler(Intersection_State *SV, tw_bf *CV, Msg_Data *M, tw_lp *lp) 
{
	tw_rand_reverse_unif(lp->rng);
	int curY = lp->gid / NUM_CELLS_X;
	int curX = lp->gid - (curY * NUM_CELLS_X);
	switch(M->event_type) 
	{
	case ARRIVAL: 
	{
		if(M->car.xDest == curX  && M->car.yDest == curY) {
			SV->total_cars_finished--;
			break;
		}
		SV->total_cars_arrived--;

		int newLane = (M->car.current_lane + 6) % 12;	//These lanes are offset by 6.
		SV->inLane[newLane]--;
		break;
	}
	case DEPARTURE: 
	{	
		SV->outLane[M->car.current_lane]++;
	
		break;
	}
	case DIRECTION_SELECT:
	{
		int sent_back=0;
		switch(M->car.current_lane % 4){

		case 0:		//Lanes with this value are all going South, ending up in lane 9,10,or 11
			if(M->car.xDest < curX && SV->outLane[11]-1  < MAX_CARS_ON_ROAD){
				SV->outLane[11]--;
			}
			else if(M->car.xDest > curX && SV->outLane[9]-1 < MAX_CARS_ON_ROAD){
				SV->outLane[9]--;
			}
			else if(SV->outLane[10]-1 < MAX_CARS_ON_ROAD){
				SV->outLane[10]--;
			}

			else{			
				sent_back =1;	
			}
			break;

		case 1:		//Lanes with this value are all going West, ending up in lane 6,7,or 8
			if(M->car.yDest < curY && SV->outLane[8]-1  < MAX_CARS_ON_ROAD){
				SV->outLane[8]--;
			}
			else if(M->car.yDest > curY && SV->outLane[6]-1 < MAX_CARS_ON_ROAD){
				SV->outLane[6]--;
			}
			else if(SV->outLane[7]-1 < MAX_CARS_ON_ROAD){
				SV->outLane[7]--;
			}

			else{			
				sent_back =1;	
			}
			break;

		case 2:		//Lanes with this value are all going North, ending up in lane 3,4,or 5
			if(M->car.xDest < curX && SV->outLane[3]-1  < MAX_CARS_ON_ROAD){
				SV->outLane[3]--;
			}
			else if(M->car.xDest > curX && SV->outLane[5]-1 < MAX_CARS_ON_ROAD){
				SV->outLane[5]--;
			}
			else if(SV->outLane[4]-1 < MAX_CARS_ON_ROAD){
				SV->outLane[4]--;
			}

			else{			
				sent_back =1;	
			}
			break;

		case 3:		//Lanes with this value are all going East, ending up in lane 0,1,or 2
			if(M->car.yDest < curY && SV->outLane[0]-1  < MAX_CARS_ON_ROAD){
				SV->outLane[0]--;
			}
			else if(M->car.yDest > curY && SV->outLane[2]-1 < MAX_CARS_ON_ROAD){
				SV->outLane[2]--;
			}
			else if(SV->outLane[1]-1 < MAX_CARS_ON_ROAD){
				SV->outLane[1]--;
			}

			else{			
				sent_back =1;	
			}
			break;
		}
		if(!sent_back)
			SV->inLane[M->car.current_lane]++;
		break;
	}
	}
}

void Intersection_Statistics_CollectStats(Intersection_State *SV, tw_lp * lp) {
	totalCars += SV->total_cars_arrived;
	carsFinished += SV->total_cars_finished;
}
