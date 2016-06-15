#ifndef INC_traffic_h
#define INC_traffic_h

#include<stdio.h>
#include <ross_api.h>
#include<math.h>   //used for sqrt

enum events { ARRIVAL, DEPARTURE, DIRECTION_SELECT };

enum ariv_dept {IN, OUT};

//struct desribing a car
typedef struct {
	int xDest;
	int yDest;
	int current_lane;
	enum ariv_dept in_out;
} a_car;

//message struct. consists of event type and car description
typedef struct {
	enum events event_type;
	a_car car;
} Msg_Data;

//State struct. describes intersection	//		     N	
typedef struct {			//		   I	O
	int total_cars_arrived;		//		_|543|345|_
	int total_cars_finished;	//	 O      876	   210	I
	int inLane[12];			//	W	_	    _		E
	int outLane[12];		//	 I    678	    012	O	
} Intersection_State;			//		_	     _			
					//		 |11109|91011|
static int g_traffic_start_events = 5;	//		   O	 I		
					//		       S
// rate for timestamp exponential distribution
static tw_stime mean = 1.0;

static char run_id[1024] = "undefined";
static unsigned long long totalCars=0;
static unsigned long long carsFinished=0;

static unsigned INTERSECTION_LPS = 1024;
static unsigned  MAX_CARS_ON_ROAD =  5;
static unsigned NUM_CELLS_X = 32;        //256     //1024
static unsigned  NUM_CELLS_Y  = 32;        //256     //1024

static unsigned  NUM_VP_X  = 8;     //32       
static unsigned  NUM_VP_Y=  8;     //32 

static tw_lpid g_cells_per_vp_x = NUM_CELLS_X/NUM_VP_X;
static tw_lpid g_cells_per_vp_y = NUM_CELLS_Y/NUM_VP_Y;
static unsigned  g_cells_per_vp = (NUM_CELLS_X/NUM_VP_X)*(NUM_CELLS_Y/NUM_VP_Y);

// Configuration parameters for setting source and destination of each car
static tw_stime g_percentStart = 0.0;
static unsigned g_startSize = 1;
static unsigned g_startX = 0;
static unsigned g_startY= 0;
static tw_stime g_percentEnd = 0.0;
static unsigned g_endSize = 1;
static unsigned g_endX = 0;
static unsigned g_endY= 0;

unsigned (*lp_source_map)(tw_lpid lp);


void Intersection_EventHandler(Intersection_State *, tw_bf *, Msg_Data *, tw_lp *);
void Intersection_RC_EventHandler(Intersection_State *, tw_bf *, Msg_Data *, tw_lp *);
void Intersection_Statistics_CollectStats(Intersection_State *, tw_lp *);
void  Intersection_StartUp(Intersection_State *SV, tw_lp * lp); 
void Intersection_Commit_Handler(Intersection_State *SV, tw_bf *CV, Msg_Data *M, tw_lp *lp);

#endif
