#include "traffic.h"


//tw_lpid g_vp_per_proc =0; // set in main
tw_lpid g_cells_per_vp_x = NUM_CELLS_X/NUM_VP_X;
tw_lpid g_cells_per_vp_y = NUM_CELLS_Y/NUM_VP_Y;
tw_lpid g_cells_per_vp = (NUM_CELLS_X/NUM_VP_X)*(NUM_CELLS_Y/NUM_VP_Y);


//TENTATTIVE DESCRIPTION

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
		n_y = ((lpid_y - 1) + NUM_CELLS_Y) % NUM_CELLS_Y;
		break;

	case 3:
		//North
		n_x = lpid_x;
		n_y = (lpid_y + 1) % NUM_CELLS_Y;
		break;

	default:
		tw_error( TW_LOC, "Bad direction value \n");
	}

	dest_lpid = (tw_lpid) (n_x + (n_y * NUM_CELLS_X));
	// printf("ComputeMove: Src LP %llu (%d, %d), Dir %u, Dest LP %llu (%d, %d)\n", lpid, lpid_x, lpid_y, direction, dest_lpid, n_x, n_y);
	return( dest_lpid );
}
/*
tw_peid
	CellMapping_lp_to_pe(tw_lpid lpid) 
{
	long lp_x = lpid % NUM_CELLS_X;
	long lp_y = lpid / NUM_CELLS_X;
	long vp_num_x = lp_x/g_cells_per_vp_x;
	long vp_num_y = lp_y/g_cells_per_vp_y;
	long vp_num = vp_num_x + (vp_num_y*NUM_VP_X);  
	tw_peid peid = vp_num/g_vp_per_proc;  
	return peid;
}
*/
//CHARE MAP (TENTATIVE)
unsigned  traffic_chare_map( tw_lpid lpid)
{
	tw_lpid lp_x = lpid % NUM_CELLS_X; //lpid -> (lp_x,lp_y)
	tw_lpid lp_y = lpid / NUM_CELLS_X;
	tw_lpid vp_index_x = lp_x / g_cells_per_vp_x;
	tw_lpid vp_index_y = lp_y / g_cells_per_vp_y;
	tw_lpid vp_index = vp_index_x + (vp_index_y * (NUM_VP_X));
	
	return (unsigned) vp_index;
}

// LOCAL MAP( TENTATIVE)

tw_lpid traffic_local_map( tw_lpid lpid)
{

	tw_lpid lp_x = lpid % NUM_CELLS_X; //lpid -> (lp_x,lp_y)
	tw_lpid lp_y = lpid / NUM_CELLS_X;
	tw_lpid vp_index_x = lp_x % g_cells_per_vp_x;
	tw_lpid vp_index_y = lp_y % g_cells_per_vp_y;
	tw_lpid vp_index = vp_index_x + (vp_index_y * (g_cells_per_vp_x));
	
	return vp_index;
}
/*
tw_lp *CellMapping_to_lp(tw_lpid lpid) 
{
	tw_lpid lp_x = lpid % NUM_CELLS_X; //lpid -> (lp_x,lp_y)
	tw_lpid lp_y = lpid / NUM_CELLS_X;
	tw_lpid vp_index_x = lp_x % g_cells_per_vp_x;
	tw_lpid vp_index_y = lp_y % g_cells_per_vp_y;
	tw_lpid vp_index = vp_index_x + (vp_index_y * (g_cells_per_vp_x));
	tw_lpid vp_num_x = lp_x/g_cells_per_vp_x;
	tw_lpid vp_num_y = lp_y/g_cells_per_vp_y;
	tw_lpid vp_num = vp_num_x + (vp_num_y*NUM_VP_X);  
	vp_num = vp_num % g_vp_per_proc;
	tw_lpid index = vp_index + vp_num*g_cells_per_vp;

#ifdef ROSS_runtime_check  
	if( index >= g_tw_nlp )
		tw_error(TW_LOC, "index (%llu) beyond g_tw_nlp (%llu) range \n", index, g_tw_nlp);
#endif   ROSS_runtime_check 

	return g_tw_lp[index];
}

tw_lpid CellMapping_to_local_index(tw_lpid lpid) 
{
	tw_lpid lp_x = lpid % NUM_CELLS_X; //lpid -> (lp_x,lp_y)
	tw_lpid lp_y = lpid / NUM_CELLS_X;
	tw_lpid vp_index_x = lp_x % g_cells_per_vp_x;
	tw_lpid vp_index_y = lp_y % g_cells_per_vp_y;
	tw_lpid vp_index = vp_index_x + (vp_index_y * (g_cells_per_vp_x));
	tw_lpid vp_num_x = lp_x/g_cells_per_vp_x;
	tw_lpid vp_num_y = lp_y/g_cells_per_vp_y;
	tw_lpid vp_num = vp_num_x + (vp_num_y*NUM_VP_X);  
	vp_num = vp_num % g_vp_per_proc;
	tw_lpid index = vp_index + vp_num*g_cells_per_vp;

	if( index >= g_tw_nlp )
		tw_error(TW_LOC, "index (%llu) beyond g_tw_nlp (%llu) range \n", index, g_tw_nlp);

	return( index );
}
*/
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
/*
void traffic_grid_mapping()
{
	tw_lpid         x, y;
	tw_lpid         lpid, kpid;
	tw_lpid         num_cells_per_kp, vp_per_proc;
	tw_lpid         local_lp_count;

	num_cells_per_kp = (NUM_CELLS_X * NUM_CELLS_Y) / (NUM_VP_X * NUM_VP_Y);
	vp_per_proc = (NUM_VP_X * NUM_VP_Y) / ((tw_nnodes() * g_tw_npe)) ;
	g_tw_nlp = nlp_per_pe;
	g_tw_nkp = vp_per_proc;

	local_lp_count=0;
	for (y = 0; y < NUM_CELLS_Y; y++)
	{
		for (x = 0; x < NUM_CELLS_X; x++)
		{
			lpid = (x + (y * NUM_CELLS_X));
			if( g_tw_mynode == CellMapping_lp_to_pe(lpid) )
			{

				kpid = local_lp_count/num_cells_per_kp;
				local_lp_count++; // MUST COME AFTER!! DO NOT PRE-INCREMENT ELSE KPID is WRONG!!

				if( kpid >= g_tw_nkp )
					tw_error(TW_LOC, "Attempting to mapping a KPid (%llu) for Global LPid %llu that is beyond g_tw_nkp (%llu)\n",
					kpid, lpid, g_tw_nkp );

				tw_lp_onpe(CellMapping_to_local_index(lpid), g_tw_pe[0], lpid);
				if( g_tw_kp[kpid] == NULL )
					tw_kp_onpe(kpid, g_tw_pe[0]);
				tw_lp_onkp(g_tw_lp[CellMapping_to_local_index(lpid)], g_tw_kp[kpid]);
				tw_lp_settype( CellMapping_to_local_index(lpid), &mylps[0]);
			}
		}
	}
}
*/
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
  TWOPT_STIME("remote", percent_remote, "desired remote event rate"),
  TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
  TWOPT_UINT("start-events", g_traffic_start_events, "number of initial messages per LP"),
  TWOPT_UINT("stagger", stagger, "Set to 1 to stagger event uniformly across 0 to end time."),
  TWOPT_CHAR("run", run_id, "user supplied run name"),
  TWOPT_END()
};






int main(int argc, char * argv[]) 
{
	g_tw_ts_end = 30;
	g_tw_gvt_interval = 16;
	int i;

	// get rid of error if compiled w/ MEMORY queues
	tw_opt_add(app_opt);
	tw_init(&argc, &argv);

	if( g_tw_lookahead > 1.0 )
		tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");

	//reset mean based on lookahead
	mean = mean - g_tw_lookahead;
/*
	offset_lpid = g_tw_mynode * nlp_per_pe;
	ttl_lps = tw_nnodes() * g_tw_npe * nlp_per_pe;
	//g_tw_rng_default = TW_FALSE;

	nlp_per_pe = (NUM_CELLS_X * NUM_CELLS_Y) / (tw_nnodes() * g_tw_npe);
	g_tw_events_per_pe = (mult * nlp_per_pe * g_traffic_start_events) + 
		optimistic_memory;
	num_cells_per_kp = (NUM_CELLS_X * NUM_CELLS_Y) / (NUM_VP_X * NUM_VP_Y);
	vp_per_proc = (NUM_VP_X * NUM_VP_Y) / ((tw_nnodes() * g_tw_npe)) ;
	g_vp_per_proc = vp_per_proc;
g_tw_nlp = nlp_per_pe;
	g_tw_nkp = vp_per_proc;


*/
	//not sure if these are needed. 
	//I'm assuming chare = VP

	g_num_chares = NUM_VP_X * NUM_VP_Y;
	g_lps_per_chare = g_cells_per_vp;
	g_total_lps = NUM_CELLS_X * NUM_CELLS_Y;

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
		printf("   stagger................%u\n", stagger);
		printf("   Mean...................%lf\n", mean);
		printf("   Mult...................%lf\n", mult);
		printf("   Memory.................%u\n", optimistic_memory);
		printf("   Remote.................%lf\n", percent_remote);
		printf("========================================\n\n");
	}

	tw_run();
	tw_end();

	printf("Number of Arivals: %lld\n", totalCars);
	printf("Number of Cars reached their dest: %lld\n", carsFinished);

	return 0;
}

void  Intersection_StartUp(Intersection_State *SV, tw_lp * lp) {
	//printf("begin init\n");
	int i;
	tw_event *CurEvent;
	tw_stime ts;
	Msg_Data *NewM;

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

	for(i = 0; i < g_traffic_start_events; i++) 
	{
		ts = tw_rand_exponential(lp->rng, MEAN_SERVICE );
		CurEvent = tw_event_new(lp->gid, ts, lp);
		NewM = (Msg_Data *)tw_event_data(CurEvent);
		NewM->event_type = ARIVAL;
		NewM->car.x_to_go =tw_rand_integer(lp->rng,0,199) - 99;
		NewM->car.y_to_go = tw_rand_integer(lp->rng,0,199) - 99;
		NewM->car.current_lane = static_cast<abs_directions> (tw_rand_integer(lp->rng,0,11));
		NewM->car.sent_back = 0;
		NewM->car.in_out = IN;
		tw_event_send(CurEvent);
	}
}

void Intersection_EventHandler(Intersection_State *SV, tw_bf *CV, Msg_Data *M, tw_lp *lp) 
{
	tw_stime ts=0.0;
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

		ts = tw_rand_exponential(lp->rng, MEAN_SERVICE);
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
		ts = tw_rand_exponential(lp->rng, MEAN_SERVICE);
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
		ts = tw_rand_exponential(lp->rng, MEAN_SERVICE);
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
