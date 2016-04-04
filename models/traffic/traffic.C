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
		//West
		n_x = ((lpid_x - 1) + NUM_CELLS_X) % NUM_CELLS_X;
		n_y = lpid_y;
		break;

	case 1:
		//East
		n_x = (lpid_x + 1) % NUM_CELLS_X;
		n_y = lpid_y;
		break;

	case 2:
		//South
		n_x = lpid_x;
		n_y = (lpid_y + 1) % NUM_CELLS_Y;
		break;

	case 3:
		//North
		n_x = lpid_x;
		n_y = ((lpid_y - 1) + NUM_CELLS_Y) % NUM_CELLS_Y;
		break;

	default:
		tw_error( TW_LOC, "Bad direction value \n");
	}

	dest_lpid = (tw_lpid) (n_x + (n_y * NUM_CELLS_X));
	// printf("ComputeMove: Src LP %llu (%d, %d), Dir %u, Dest LP %llu (%d, %d)\n", lpid, lpid_x, lpid_y, direction, dest_lpid, n_x, n_y);
	return( dest_lpid );
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
  TWOPT_UINT("balance",g_balance, "distribution of cars: 0 = balanced, 1=imbalanced"),
  TWOPT_STIME("percentStart",g_percentStart, "Pecent of cars that start in clustered block [0-1]"),
  TWOPT_UINT("startSize",g_startSize, "Size of start cluster block. X by X block"),
  TWOPT_UINT("startX",g_startX, "X coord of upper left corner of start cluster"),
  TWOPT_UINT("startY",g_startY, "Y coord of upper left corner of start cluster"),
   TWOPT_STIME("percentEnd",g_percentEnd, "Pecent of cars that end in clustered block [0-1]"),
  TWOPT_UINT("destSize",g_endSize, "Size of end cluster block. X by X block"),
  TWOPT_UINT("destX",g_endX, "X coord of upper left corner of end cluster"),
  TWOPT_UINT("destY",g_endY, "Y coord of upper left corner of end cluster"),
   TWOPT_UINT("start-events", g_traffic_start_events, "average start events per LP. start-events * numLPS = total start events"),
//  TWOPT_UINT("total-lps", INTERSECTION_LPS, "Total LPS (intersections) in simulation. Should be perfect square"),
//  TWOPT_UINT("lpPerChare", g_cells_per_vp, "Number of lps per chare. Should be perfect square"),
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
	tw_end();

	printf("Number of Arivals: %lld\n", totalCars);
	printf("Number of Cars reached their dest: %lld\n", carsFinished);

	return 0;
}

void  Intersection_StartUp(Intersection_State *SV, tw_lp * lp) {
//	printf("begin init\n");

	SV->total_cars_arrived = 0;
	SV->total_cars_finished = 0;
	SV->num_in_west_left = 0;
	SV->num_in_west_straight = 0;
	SV->num_in_west_right = 0;
	SV->num_in_north_left = 0;
	SV->num_in_north_straight = 0;
	SV->num_in_north_right = 0;
	SV->num_in_south_left = 0;
	SV->num_in_south_straight = 0;
	SV->num_in_south_right = 0;
	SV->num_in_east_left = 0;
	SV->num_in_east_straight = 0;
	SV->num_in_east_right = 0;
	SV->num_out_west_left = 0;
	SV->num_out_west_straight = 0;
	SV->num_out_west_right = 0;
	SV->num_out_north_left = 0;
	SV->num_out_north_straight = 0;
	SV->num_out_north_right = 0;
	SV->num_out_south_left = 0;
	SV->num_out_south_straight = 0;
	SV->num_out_south_right = 0;
	SV->num_out_east_left = 0;
	SV->num_out_east_straight = 0;
	SV->num_out_east_right = 0;

	int i = 0;
	tw_event *CurEvent;
	tw_stime ts = 0;
	Msg_Data *NewM;
	tw_lpid dest = 0;			//used to pick start location
	tw_lpid destX = 0;
	tw_lpid destY = 0;
	tw_lpid endX = 0;
	tw_lpid endY = 0;
	switch(g_balance) {
	
	case 0: 				//balanced distribution
		for(i = 0; i < g_traffic_start_events; i++) 
		{
			ts = g_tw_lookahead + tw_rand_exponential(lp->rng, mean);
			CurEvent = tw_event_new(lp->gid, ts, lp);
			NewM = (Msg_Data *)tw_event_data(CurEvent);
			NewM->event_type = ARIVAL;
			NewM->car.x_to_go =tw_rand_integer(lp->rng,0,198) - 99;		//distance for car to travel. ranges from -99 to 99.
			NewM->car.y_to_go = tw_rand_integer(lp->rng,0,198) - 99;
			NewM->car.current_lane = static_cast<abs_directions> (tw_rand_integer(lp->rng,0,11));
			NewM->car.sent_back = 0;
			NewM->car.in_out = IN;
			tw_event_send(CurEvent);
		}
		break;
	
	case 1:					//unbalanced
	
		for(i = 0; i < g_traffic_start_events; i++) 
		{
			if( g_percentStart < tw_rand_unif(lp->rng))
			{	
				dest = tw_rand_integer(lp->rng,0,INTERSECTION_LPS-1);
				destX = dest % NUM_CELLS_X;
				destY = dest / NUM_CELLS_X;
			}
			else
			{
				destX = tw_rand_integer(lp->rng,0,g_startSize-1)+ g_startX;
				destY = tw_rand_integer(lp->rng,0,g_startSize-1)+g_startY;
				dest = destX + NUM_CELLS_X * destY;
			}
			ts = g_tw_lookahead + tw_rand_exponential(lp->rng, mean);
			CurEvent = tw_event_new(dest, ts, lp);
			NewM = (Msg_Data *)tw_event_data(CurEvent);
			NewM->event_type = ARIVAL;

			if( g_percentEnd < tw_rand_unif(lp->rng))
			{
				NewM->car.x_to_go =tw_rand_integer(lp->rng,0,198) - 99;		//distance for car to travel. ranges from -99 to 99.
				NewM->car.y_to_go = tw_rand_integer(lp->rng,0,198) - 99;
			}
			else
			{
				endX = tw_rand_integer(lp->rng,0,g_endSize-1)+ g_endX;
				endY = tw_rand_integer(lp->rng,0,g_endSize-1)+g_endY;
				NewM->car.x_to_go = endX-destX;		
				NewM->car.y_to_go = endY-destY;

			}
			
				
			NewM->car.current_lane = static_cast<abs_directions> (tw_rand_integer(lp->rng,0,11));
			NewM->car.sent_back = 0;
			NewM->car.in_out = IN;
			tw_event_send(CurEvent);
		}
		break;
	
	}
}

void Intersection_EventHandler(Intersection_State *SV, tw_bf *CV, Msg_Data *M, tw_lp *lp) 
{
	
	tw_stime ts = g_tw_lookahead;
	int new_event_direction=0;
	tw_event *CurEvent=NULL;
	Msg_Data *NewM=NULL;
//	enum abs_directions temp_direction=0;   NOT SURE WHAT ISSUE IS
	enum abs_directions temp_direction;
	*(int *)CV = (int)0;

	switch(M->event_type) {

	case ARIVAL: 

		if(M->car.x_to_go == 0 && M->car.y_to_go == 0){
			SV->total_cars_finished++;
			break;
		}

		// Schedule a departure in the future
		SV->total_cars_arrived++;

		switch(M->car.current_lane){

		case WEST_LEFT:
			SV->num_in_east_left++;	
			M->car.current_lane = EAST_LEFT;
			break;
		case WEST_STRAIGHT:
			SV->num_in_east_straight++;	
			M->car.current_lane = EAST_STRAIGHT;
			break;
		case WEST_RIGHT: 
			SV->num_in_east_right++;
			M->car.current_lane = EAST_RIGHT;
			break;
		case EAST_LEFT: 
			SV->num_in_west_left++;
			M->car.current_lane = WEST_LEFT;
			break;
		case EAST_STRAIGHT: 
			SV->num_in_west_straight++;
			M->car.current_lane = WEST_STRAIGHT;
			break;
		case EAST_RIGHT: 
			SV->num_in_west_right++;
			M->car.current_lane = WEST_RIGHT;
			break;
		case NORTH_LEFT: 
			SV->num_in_south_left++;
			M->car.current_lane = SOUTH_LEFT;
			break;
		case NORTH_STRAIGHT: 
			SV->num_in_south_straight++;
			M->car.current_lane = SOUTH_STRAIGHT;
			break;
		case NORTH_RIGHT: 
			SV->num_in_south_right++;
			M->car.current_lane = SOUTH_RIGHT;
			break;
		case SOUTH_LEFT:
			SV->num_in_north_left++;
			M->car.current_lane = NORTH_LEFT;	
			break; 
		case SOUTH_STRAIGHT:
			SV->num_in_north_straight++;
			M->car.current_lane = NORTH_STRAIGHT;
			break; 
		case SOUTH_RIGHT:
			SV->num_in_north_right++;
			M->car.current_lane = NORTH_RIGHT;
			break;
		}

		M->car.in_out = IN;

		ts += tw_rand_exponential(lp->rng, mean);
		CurEvent = tw_event_new(lp->gid, ts, lp);
		NewM = (Msg_Data *)tw_event_data(CurEvent);
		NewM->car.x_to_go = M->car.x_to_go;
		NewM->car.y_to_go = M->car.y_to_go;
		NewM->car.current_lane = M->car.current_lane;
		NewM->car.sent_back = M->car.sent_back;
		NewM->car.arrived_from = M->car.arrived_from;
		NewM->car.in_out = M->car.in_out;
		NewM->event_type = DIRECTION_SELECT;
		//printf("send ari ");
		tw_event_send(CurEvent);


		break;

	case DEPARTURE: 

		switch(M->car.current_lane){
		case WEST_LEFT:
			SV->num_out_west_left--;
			new_event_direction = 0;
			break;
		case WEST_STRAIGHT:
			SV->num_out_west_straight--;
			new_event_direction = 0;
			break;
		case WEST_RIGHT: 
			SV->num_out_west_right--;
			new_event_direction = 0;
			break;
		case EAST_LEFT:
			SV->num_out_east_left--;
			new_event_direction = 1;
			break;
		case EAST_STRAIGHT: 
			SV->num_out_east_straight--;
			new_event_direction = 1;
			break;
		case EAST_RIGHT: 
			SV->num_out_east_right--;
			new_event_direction = 1;
			break;
		case NORTH_LEFT:
			SV->num_out_north_left--;
			new_event_direction = 3;
			break;
		case NORTH_STRAIGHT:
			SV->num_out_north_straight--;
			new_event_direction = 3;
			break;
		case NORTH_RIGHT:
			SV->num_out_north_right--;
			new_event_direction = 3;
			break;
		case SOUTH_LEFT:
			SV->num_out_south_left--;
			new_event_direction = 2;
			break;
		case SOUTH_STRAIGHT:
			SV->num_out_south_straight--;
			new_event_direction = 2;
			break;
		case SOUTH_RIGHT:
			SV->num_out_south_right--;
			new_event_direction = 2;
			break;
		}

		lp->gid = Cell_ComputeMove(lp->gid, new_event_direction);
		ts += tw_rand_exponential(lp->rng, mean);
		CurEvent = tw_event_new(lp->gid, ts, lp);
		NewM = (Msg_Data *) tw_event_data(CurEvent);
		NewM->car.x_to_go = M->car.x_to_go;
		NewM->car.y_to_go = M->car.y_to_go;
		NewM->car.current_lane = M->car.current_lane;
		NewM->car.sent_back = M->car.sent_back;
		NewM->car.arrived_from = M->car.arrived_from;
		NewM->car.in_out = M->car.in_out;
		NewM->event_type = ARIVAL;
		//printf("send dep ");
		tw_event_send(CurEvent);
		break;

	case DIRECTION_SELECT:


		temp_direction = M->car.current_lane;

		switch(M->car.current_lane){
		case EAST_LEFT:
			SV->num_in_east_left--;
			if(M->car.y_to_go < 0 && SV->num_out_south_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = SOUTH_STRAIGHT;
				SV->num_out_south_straight ++;
				M->car.sent_back = 0;
				M->car.y_to_go++;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_south_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = SOUTH_RIGHT;
				SV->num_out_south_right ++;
				M->car.sent_back = 0;
				M->car.x_to_go++;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_south_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = SOUTH_LEFT;
				SV->num_out_south_left ++;
				M->car.sent_back = 0;
				M->car.x_to_go--;
			}
			else{
				if(M->car.arrived_from == SOUTH_LEFT){
					M->car.current_lane = EAST_RIGHT;
					SV->num_out_east_right++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == EAST_STRAIGHT){
					M->car.current_lane = EAST_STRAIGHT;
					SV->num_out_east_straight++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == NORTH_RIGHT){
					M->car.current_lane = EAST_LEFT;
					SV->num_out_east_left++;
					M->car.sent_back++;
				}
			}
			break;
		case EAST_STRAIGHT:
			SV->num_in_east_straight--;
			if(M->car.x_to_go < 0 && SV->num_out_west_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = WEST_STRAIGHT;
				SV->num_out_west_straight ++;
				M->car.sent_back = 0;
				M->car.x_to_go++;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_west_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = WEST_LEFT;
				SV->num_out_west_left ++;
				M->car.sent_back = 0;
				M->car.y_to_go++;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_west_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = WEST_RIGHT;
				SV->num_out_west_right ++;
				M->car.sent_back = 0;
				M->car.y_to_go--;
			}
			else{
				if(M->car.arrived_from == NORTH_RIGHT){
					M->car.current_lane = EAST_LEFT;
					SV->num_out_east_left++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == EAST_STRAIGHT){
					M->car.current_lane = EAST_STRAIGHT;
					SV->num_out_east_straight++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == SOUTH_LEFT){
					M->car.current_lane = EAST_RIGHT;
					SV->num_out_east_right++;
					M->car.sent_back++;
				}
			}

			break;
		case EAST_RIGHT: 
			SV->num_in_east_right--;
			if(M->car.y_to_go > 0 && SV->num_out_north_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = NORTH_STRAIGHT;
				SV->num_out_north_straight ++;
				M->car.sent_back = 0;
				M->car.y_to_go--;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_north_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = NORTH_RIGHT;
				SV->num_out_north_right ++;
				M->car.sent_back = 0;
				M->car.x_to_go --;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_north_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = NORTH_LEFT;
				SV->num_out_north_left ++;
				M->car.sent_back = 0;
				M->car.x_to_go++;
			}
			else{
				if(M->car.arrived_from == SOUTH_LEFT){
					M->car.current_lane = EAST_RIGHT;
					SV->num_out_east_right++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == EAST_STRAIGHT){
					M->car.current_lane = EAST_STRAIGHT;
					SV->num_out_east_straight++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == NORTH_RIGHT){
					M->car.current_lane = EAST_LEFT;
					SV->num_out_east_left++;
					M->car.sent_back++;
				}
			}
			break;
		case WEST_LEFT:
			SV->num_in_west_left--;
			if(M->car.y_to_go > 0 && SV->num_out_north_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = NORTH_STRAIGHT;
				SV->num_out_north_straight ++;
				M->car.sent_back = 0;
				M->car.y_to_go--;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_north_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = NORTH_RIGHT;
				SV->num_out_north_right ++;
				M->car.sent_back = 0;
				M->car.x_to_go--;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_north_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = NORTH_LEFT;
				SV->num_out_north_left ++;
				M->car.sent_back = 0;
				M->car.x_to_go++;
			}
			else{
				if(M->car.arrived_from == SOUTH_RIGHT){
					M->car.current_lane = WEST_LEFT;
					SV->num_out_west_left++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == WEST_STRAIGHT){
					M->car.current_lane = WEST_STRAIGHT;
					SV->num_out_west_straight++;							
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == NORTH_LEFT){
					M->car.current_lane = WEST_RIGHT;
					SV->num_out_west_right++;
					M->car.sent_back++;
				}
			}
			break;
		case WEST_STRAIGHT: 
			SV->num_in_west_straight--;
			if(M->car.x_to_go > 0 && SV->num_out_east_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = EAST_STRAIGHT;
				SV->num_out_east_straight ++;
				M->car.sent_back = 0;
				M->car.x_to_go--;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_east_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = EAST_LEFT;
				SV->num_out_east_left ++;
				M->car.sent_back = 0;
				M->car.y_to_go --;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_east_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = EAST_RIGHT;
				SV->num_out_east_right ++;
				M->car.sent_back = 0;
				M->car.y_to_go++;
			}
			else{
				if(M->car.arrived_from == SOUTH_RIGHT){
					M->car.current_lane = WEST_LEFT;
					SV->num_out_west_left++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == WEST_STRAIGHT){
					M->car.current_lane = WEST_STRAIGHT;
					SV->num_out_west_straight++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == NORTH_LEFT){
					M->car.current_lane = WEST_RIGHT;
					SV->num_out_west_right++;
					M->car.sent_back++;
				}
			}
			break;
		case WEST_RIGHT: 
			SV->num_in_west_right--;
			if(M->car.y_to_go < 0 && SV->num_out_south_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = SOUTH_STRAIGHT;
				SV->num_out_south_straight ++;
				M->car.sent_back = 0;
				M->car.y_to_go++;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_south_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = SOUTH_LEFT;
				SV->num_out_south_left ++;
				M->car.sent_back = 0;
				M->car.x_to_go--;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_south_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = SOUTH_RIGHT;
				SV->num_out_south_right ++;
				M->car.sent_back = 0;
				M->car.x_to_go++;
			}
			else{
				if(M->car.arrived_from == SOUTH_RIGHT){
					M->car.current_lane = WEST_LEFT;
					SV->num_out_west_left++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == WEST_STRAIGHT){
					M->car.current_lane = WEST_STRAIGHT;
					SV->num_out_west_straight++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == NORTH_LEFT){
					M->car.current_lane = WEST_RIGHT;
					SV->num_out_west_right++;
					M->car.sent_back++;
				}
			}
			break;
		case NORTH_LEFT: 
			SV->num_in_north_left--;
			if(M->car.x_to_go > 0 && SV->num_out_east_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = EAST_STRAIGHT;
				SV->num_out_east_straight ++;
				M->car.sent_back = 0;
				M->car.x_to_go --;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_east_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = EAST_LEFT;
				SV->num_out_east_left ++;
				M->car.sent_back = 0;
				M->car.y_to_go--;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_east_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = EAST_RIGHT;
				SV->num_out_east_right ++;
				M->car.sent_back = 0;
				M->car.y_to_go++;
			}
			else{
				if(M->car.arrived_from == WEST_RIGHT){
					M->car.current_lane = NORTH_LEFT;
					SV->num_out_north_left++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == NORTH_STRAIGHT){
					M->car.current_lane = NORTH_STRAIGHT;
					SV->num_out_north_straight++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == EAST_LEFT){
					M->car.current_lane = NORTH_RIGHT;
					SV->num_out_north_right++;
					M->car.sent_back++;
				}
			}

			break;
		case NORTH_STRAIGHT:
			SV->num_in_north_straight--;
			if(M->car.y_to_go < 0 && SV->num_out_south_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = SOUTH_STRAIGHT;
				SV->num_out_south_straight ++;
				M->car.sent_back = 0;
				M->car.y_to_go++;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_south_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = SOUTH_LEFT;
				SV->num_out_south_left ++;
				M->car.sent_back = 0;
				M->car.x_to_go--;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_south_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = SOUTH_RIGHT;
				SV->num_out_south_right ++;
				M->car.sent_back = 0;
				M->car.x_to_go++;
			}
			else{
				if(M->car.arrived_from == WEST_RIGHT){
					M->car.current_lane = NORTH_LEFT;
					SV->num_out_north_left++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == NORTH_STRAIGHT){
					M->car.current_lane = NORTH_STRAIGHT;
					SV->num_out_north_straight++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == EAST_LEFT){
					M->car.current_lane = NORTH_RIGHT;
					SV->num_out_north_right++;
					M->car.sent_back++;
				}
			}
			break;
		case NORTH_RIGHT: 
			SV->num_in_north_right--;
			if(M->car.x_to_go < 0 && SV->num_out_west_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = WEST_STRAIGHT;
				SV->num_out_west_straight ++;
				M->car.sent_back = 0;
				M->car.x_to_go++;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_west_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = WEST_LEFT;
				SV->num_out_west_left ++;
				M->car.sent_back = 0;
				M->car.y_to_go++;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_west_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = WEST_RIGHT;
				SV->num_out_west_right ++;
				M->car.sent_back = 0;
				M->car.y_to_go--;
			}
			else{
				if(M->car.arrived_from == WEST_RIGHT){
					M->car.current_lane = NORTH_LEFT;
					SV->num_out_north_left++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == NORTH_STRAIGHT){
					M->car.current_lane = NORTH_STRAIGHT;
					SV->num_out_north_straight++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == EAST_LEFT){
					M->car.current_lane = NORTH_RIGHT;
					SV->num_out_north_right++;
					M->car.sent_back++;
				}
			}
			break;
		case SOUTH_LEFT:
			SV->num_in_south_left--;
			if(M->car.x_to_go < 0 && SV->num_out_west_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = WEST_STRAIGHT;
				SV->num_out_west_straight ++;
				M->car.sent_back = 0;
				M->car.x_to_go++;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_west_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = WEST_LEFT;
				SV->num_out_west_left ++;
				M->car.sent_back = 0;
				M->car.y_to_go++;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_west_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = WEST_RIGHT;
				SV->num_out_west_right ++;
				M->car.sent_back = 0;
				M->car.y_to_go--;
			}
			else{
				if(M->car.arrived_from == WEST_LEFT){
					M->car.current_lane = SOUTH_RIGHT;
					SV->num_out_south_right++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == SOUTH_STRAIGHT){
					M->car.current_lane = SOUTH_STRAIGHT;
					SV->num_out_south_straight++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == EAST_RIGHT){
					M->car.current_lane = SOUTH_LEFT;
					SV->num_out_south_left++;
					M->car.sent_back++;
				}
			}

			break; 
		case SOUTH_STRAIGHT:
			SV->num_in_south_straight--;
			if(M->car.y_to_go > 0 && SV->num_out_north_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = NORTH_STRAIGHT;
				SV->num_out_north_straight ++;
				M->car.sent_back = 0;
				M->car.y_to_go--;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_north_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = NORTH_LEFT;
				SV->num_out_north_left ++;
				M->car.sent_back = 0;
				M->car.x_to_go++;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_north_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = NORTH_RIGHT;
				SV->num_out_north_right ++;
				M->car.sent_back = 0;
				M->car.x_to_go --;
			}
			else{
				if(M->car.arrived_from == EAST_RIGHT){
					M->car.current_lane = SOUTH_LEFT;
					SV->num_out_south_left++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == SOUTH_STRAIGHT){
					M->car.current_lane = SOUTH_STRAIGHT;
					SV->num_out_south_straight++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == WEST_LEFT){
					M->car.current_lane = SOUTH_RIGHT;
					SV->num_out_south_right++;
					M->car.sent_back++;
				}
			}
			break; 
		case SOUTH_RIGHT:
			SV->num_in_south_right--;
			if(M->car.x_to_go > 0 && SV->num_out_east_straight < MAX_CARS_ON_ROAD){
				M->car.current_lane = EAST_STRAIGHT;
				SV->num_out_east_straight ++;
				M->car.sent_back = 0;
				M->car.x_to_go--;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_east_left < MAX_CARS_ON_ROAD){
				M->car.current_lane = EAST_LEFT;
				SV->num_out_east_left ++;
				M->car.sent_back = 0;
				M->car.y_to_go--;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_east_right < MAX_CARS_ON_ROAD){
				M->car.current_lane = EAST_RIGHT;
				SV->num_out_east_right ++;
				M->car.sent_back = 0;
				M->car.y_to_go++;
			}
			else{
				if(M->car.arrived_from == EAST_RIGHT){
					M->car.current_lane = SOUTH_LEFT;
					SV->num_out_south_left++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == SOUTH_STRAIGHT){
					M->car.current_lane = SOUTH_STRAIGHT;
					SV->num_out_south_straight++;
					M->car.sent_back++;
				}
				else if(M->car.arrived_from == WEST_LEFT){
					M->car.current_lane = SOUTH_RIGHT;
					SV->num_out_south_right++;
					M->car.sent_back++;
				}
			}
			break;
		}

		M->car.arrived_from = temp_direction;
		M->car.in_out = OUT;
		ts += tw_rand_exponential(lp->rng, mean);
		CurEvent = tw_event_new(lp->gid, ts, lp);
		NewM = (Msg_Data *)tw_event_data(CurEvent);
		NewM->car.x_to_go = M->car.x_to_go;
		NewM->car.y_to_go = M->car.y_to_go;
		NewM->car.current_lane = M->car.current_lane;
		NewM->car.sent_back = M->car.sent_back;
		NewM->car.arrived_from = M->car.arrived_from;
		NewM->car.in_out = M->car.in_out;
		NewM->event_type = DEPARTURE;
		//printf("send dir ");
		tw_event_send(CurEvent);
		break;
	}
}


void Intersection_RC_EventHandler(Intersection_State *SV, tw_bf *CV, Msg_Data *M, tw_lp *lp) 
{

	enum abs_directions temp_direction;
	*(int *)CV = (int)0;

	switch(M->event_type) 
	{

	case ARIVAL: 

		if(M->car.x_to_go == 0 && M->car.y_to_go == 0)
		{
			SV->total_cars_finished--;
			break;
		}

		// Schedule a departure in the future
		SV->total_cars_arrived--;

		switch(M->car.current_lane)
		{
		case WEST_LEFT:
			SV->num_in_east_left--;	
			break;
		case WEST_STRAIGHT:
			SV->num_in_east_straight--;	
			break;
		case WEST_RIGHT: 
			SV->num_in_east_right++;
			break;
		case EAST_LEFT: 
			SV->num_in_west_left++;
			break;
		case EAST_STRAIGHT: 
			SV->num_in_west_straight++;
			break;
		case EAST_RIGHT: 
			SV->num_in_west_right++;
			break;
		case NORTH_LEFT: 
			SV->num_in_south_left++;
			break;
		case NORTH_STRAIGHT: 
			SV->num_in_south_straight++;
			break;
		case NORTH_RIGHT: 
			SV->num_in_south_right++;
			break;
		case SOUTH_LEFT:
			SV->num_in_north_left++;
			break; 
		case SOUTH_STRAIGHT:
			SV->num_in_north_straight++;
			break; 
		case SOUTH_RIGHT:
			SV->num_in_north_right++;
			break;
		}

		// ts = tw_rand_exponential(lp->rng, MEAN_SERVICE);
		tw_rand_reverse_unif( lp->rng );
		break;

	case DEPARTURE: 

		switch(M->car.current_lane){
		case WEST_LEFT:
			SV->num_out_west_left++;
			break;
		case WEST_STRAIGHT:
			SV->num_out_west_straight++;
			break;
		case WEST_RIGHT: 
			SV->num_out_west_right++;
			break;
		case EAST_LEFT:
			SV->num_out_east_left++;
			break;
		case EAST_STRAIGHT: 
			SV->num_out_east_straight++;
			break;
		case EAST_RIGHT: 
			SV->num_out_east_right++;
			break;
		case NORTH_LEFT:
			SV->num_out_north_left++;
			break;
		case NORTH_STRAIGHT:
			SV->num_out_north_straight++;
			break;
		case NORTH_RIGHT:
			SV->num_out_north_right++;
			break;
		case SOUTH_LEFT:
			SV->num_out_south_left++;
			break;
		case SOUTH_STRAIGHT:
			SV->num_out_south_straight++;
			break;
		case SOUTH_RIGHT:
			SV->num_out_south_right++;
			break;
		}

		// ts = tw_rand_exponential(lp->rng, MEAN_SERVICE);
		tw_rand_reverse_unif( lp->rng );
		break;

	case DIRECTION_SELECT:


		temp_direction = M->car.current_lane;

		switch(M->car.current_lane){
		case EAST_LEFT:
			SV->num_in_east_left++;
			if(M->car.y_to_go < 0 && SV->num_out_south_straight < MAX_CARS_ON_ROAD){
				SV->num_out_south_straight --;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_south_right < MAX_CARS_ON_ROAD){
				SV->num_out_south_right --;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_south_left < MAX_CARS_ON_ROAD){
				SV->num_out_south_left --;
			}
			else{
				if(M->car.arrived_from == SOUTH_LEFT){
					SV->num_out_east_right--;
				}
				else if(M->car.arrived_from == EAST_STRAIGHT){
					SV->num_out_east_straight--;
				}
				else if(M->car.arrived_from == NORTH_RIGHT){
					SV->num_out_east_left--;
				}
			}
			break;
		case EAST_STRAIGHT:
			SV->num_in_east_straight++;
			if(M->car.x_to_go < 0 && SV->num_out_west_straight < MAX_CARS_ON_ROAD){
				SV->num_out_west_straight --;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_west_left < MAX_CARS_ON_ROAD){
				SV->num_out_west_left --;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_west_right < MAX_CARS_ON_ROAD){
				SV->num_out_west_right --;
			}
			else{
				if(M->car.arrived_from == NORTH_RIGHT){
					SV->num_out_east_left--;
				}
				else if(M->car.arrived_from == EAST_STRAIGHT){
					SV->num_out_east_straight--;
				}
				else if(M->car.arrived_from == SOUTH_LEFT){
					SV->num_out_east_right++;
				}
			}

			break;
		case EAST_RIGHT: 
			SV->num_in_east_right++;
			if(M->car.y_to_go > 0 && SV->num_out_north_straight < MAX_CARS_ON_ROAD){
				SV->num_out_north_straight --;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_north_right < MAX_CARS_ON_ROAD){
				SV->num_out_north_right --;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_north_left < MAX_CARS_ON_ROAD){
				SV->num_out_north_left --;
			}
			else{
				if(M->car.arrived_from == SOUTH_LEFT){
					SV->num_out_east_right--;
				}
				else if(M->car.arrived_from == EAST_STRAIGHT){
					SV->num_out_east_straight--;
				}
				else if(M->car.arrived_from == NORTH_RIGHT){
					SV->num_out_east_left--;
				}
			}
			break;
		case WEST_LEFT:
			SV->num_in_west_left++;
			if(M->car.y_to_go > 0 && SV->num_out_north_straight < MAX_CARS_ON_ROAD){
				SV->num_out_north_straight --;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_north_right < MAX_CARS_ON_ROAD){
				SV->num_out_north_right --;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_north_left < MAX_CARS_ON_ROAD){
				SV->num_out_north_left --;
			}
			else{
				if(M->car.arrived_from == SOUTH_RIGHT){
					SV->num_out_west_left--;
				}
				else if(M->car.arrived_from == WEST_STRAIGHT){
					SV->num_out_west_straight--;							
				}
				else if(M->car.arrived_from == NORTH_LEFT){
					SV->num_out_west_right--;
				}
			}
			break;
		case WEST_STRAIGHT: 
			SV->num_in_west_straight++;
			if(M->car.x_to_go > 0 && SV->num_out_east_straight < MAX_CARS_ON_ROAD){
				SV->num_out_east_straight --;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_east_left < MAX_CARS_ON_ROAD){
				SV->num_out_east_left --;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_east_right < MAX_CARS_ON_ROAD){
				SV->num_out_east_right --;
			}
			else{
				if(M->car.arrived_from == SOUTH_RIGHT){
					SV->num_out_west_left--;
				}
				else if(M->car.arrived_from == WEST_STRAIGHT){
					SV->num_out_west_straight--;
				}
				else if(M->car.arrived_from == NORTH_LEFT){
					SV->num_out_west_right--;
				}
			}
			break;
		case WEST_RIGHT: 
			SV->num_in_west_right++;
			if(M->car.y_to_go < 0 && SV->num_out_south_straight < MAX_CARS_ON_ROAD){
				SV->num_out_south_straight --;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_south_left < MAX_CARS_ON_ROAD){
				SV->num_out_south_left --;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_south_right < MAX_CARS_ON_ROAD){
				SV->num_out_south_right --;
			}
			else{
				if(M->car.arrived_from == SOUTH_RIGHT){
					SV->num_out_west_left--;
				}
				else if(M->car.arrived_from == WEST_STRAIGHT){
					SV->num_out_west_straight--;
				}
				else if(M->car.arrived_from == NORTH_LEFT){
					SV->num_out_west_right--;
				}
			}
			break;
		case NORTH_LEFT: 
			SV->num_in_north_left++;
			if(M->car.x_to_go > 0 && SV->num_out_east_straight < MAX_CARS_ON_ROAD){
				SV->num_out_east_straight --;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_east_left < MAX_CARS_ON_ROAD){
				SV->num_out_east_left --;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_east_right < MAX_CARS_ON_ROAD){
				SV->num_out_east_right --;
			}
			else{
				if(M->car.arrived_from == WEST_RIGHT){
					SV->num_out_north_left--;
				}
				else if(M->car.arrived_from == NORTH_STRAIGHT){
					SV->num_out_north_straight--;
				}
				else if(M->car.arrived_from == EAST_LEFT){
					SV->num_out_north_right--;
				}
			}
			break;
		case NORTH_STRAIGHT:
			SV->num_in_north_straight++;
			if(M->car.y_to_go < 0 && SV->num_out_south_straight < MAX_CARS_ON_ROAD){
				SV->num_out_south_straight --;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_south_left < MAX_CARS_ON_ROAD){
				SV->num_out_south_left --;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_south_right < MAX_CARS_ON_ROAD){
				SV->num_out_south_right --;
			}
			else{
				if(M->car.arrived_from == WEST_RIGHT){
					SV->num_out_north_left--;
				}
				else if(M->car.arrived_from == NORTH_STRAIGHT){
					SV->num_out_north_straight--;
				}
				else if(M->car.arrived_from == EAST_LEFT){
					SV->num_out_north_right--;
				}
			}
			break;
		case NORTH_RIGHT: 
			SV->num_in_north_right++;
			if(M->car.x_to_go < 0 && SV->num_out_west_straight < MAX_CARS_ON_ROAD){
				SV->num_out_west_straight --;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_west_left < MAX_CARS_ON_ROAD){
				SV->num_out_west_left --;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_west_right < MAX_CARS_ON_ROAD){
				SV->num_out_west_right --;
			}
			else{
				if(M->car.arrived_from == WEST_RIGHT){
					SV->num_out_north_left--;
				}
				else if(M->car.arrived_from == NORTH_STRAIGHT){
					SV->num_out_north_straight--;
				}
				else if(M->car.arrived_from == EAST_LEFT){
					SV->num_out_north_right--;
				}
			}
			break;
		case SOUTH_LEFT:
			SV->num_in_south_left++;
			if(M->car.x_to_go < 0 && SV->num_out_west_straight < MAX_CARS_ON_ROAD){
				SV->num_out_west_straight --;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_west_left < MAX_CARS_ON_ROAD){
				SV->num_out_west_left --;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_west_right < MAX_CARS_ON_ROAD){
				SV->num_out_west_right --;
			}
			else{
				if(M->car.arrived_from == WEST_LEFT){
					SV->num_out_south_right--;
				}
				else if(M->car.arrived_from == SOUTH_STRAIGHT){
					SV->num_out_south_straight--;
				}
				else if(M->car.arrived_from == EAST_RIGHT){
					SV->num_out_south_left--;
				}
			}

			break; 
		case SOUTH_STRAIGHT:
			SV->num_in_south_straight++;
			if(M->car.y_to_go > 0 && SV->num_out_north_straight < MAX_CARS_ON_ROAD){
				SV->num_out_north_straight --;
			}
			else if(M->car.x_to_go < 0 && SV->num_out_north_left < MAX_CARS_ON_ROAD){
				SV->num_out_north_left --;
			}
			else if(M->car.x_to_go > 0 && SV->num_out_north_right < MAX_CARS_ON_ROAD){
				SV->num_out_north_right --;
			}
			else{
				if(M->car.arrived_from == EAST_RIGHT){
					SV->num_out_south_left--;
				}
				else if(M->car.arrived_from == SOUTH_STRAIGHT){
					SV->num_out_south_straight--;
				}
				else if(M->car.arrived_from == WEST_LEFT){
					SV->num_out_south_right--;
				}
			}
			break; 
		case SOUTH_RIGHT:
			SV->num_in_south_right++;
			if(M->car.x_to_go > 0 && SV->num_out_east_straight < MAX_CARS_ON_ROAD){
				SV->num_out_east_straight --;
			}
			else if(M->car.y_to_go > 0 && SV->num_out_east_left < MAX_CARS_ON_ROAD){
				SV->num_out_east_left --;
			}
			else if(M->car.y_to_go < 0 && SV->num_out_east_right < MAX_CARS_ON_ROAD){
				SV->num_out_east_right --;
			}
			else{
				if(M->car.arrived_from == EAST_RIGHT){
					SV->num_out_south_left--;
				}
				else if(M->car.arrived_from == SOUTH_STRAIGHT){
					SV->num_out_south_straight--;
				}
				else if(M->car.arrived_from == WEST_LEFT){
					SV->num_out_south_right--;
				}
			}
			break;
		}

		// ts = tw_rand_exponential(lp->rng, MEAN_SERVICE);
		tw_rand_reverse_unif( lp->rng );
		break;
	}
}

void Intersection_Statistics_CollectStats(Intersection_State *SV, tw_lp * lp) {
	totalCars += SV->total_cars_arrived;
	carsFinished += SV->total_cars_finished;
}
