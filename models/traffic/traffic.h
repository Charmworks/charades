#ifndef INC_traffic_h
#define INC_traffic_h

#include <ross_api.h>
#include<math.h>   //used for sqrt

//NOT USED IN CODE
enum events { ARIVAL, DEPARTURE, DIRECTION_SELECT };

enum abs_directions { WEST_LEFT = 0, WEST_STRAIGHT, WEST_RIGHT, EAST_LEFT, EAST_STRAIGHT, EAST_RIGHT, NORTH_LEFT, NORTH_STRAIGHT, NORTH_RIGHT, SOUTH_LEFT, SOUTH_STRAIGHT, SOUTH_RIGHT }; 
enum ariv_dept {IN, OUT};

//struct desribing a car
typedef struct {
	int x_to_go;
	int y_to_go;
	int sent_back;
	enum abs_directions arrived_from;
	enum abs_directions current_lane;
	enum ariv_dept in_out;
} a_car;

//message struct. consists of event type and car description
typedef struct {
	enum events event_type;
	a_car car;
} Msg_Data;

//State struct. describes intersection
typedef struct {
	int total_cars_arrived;
	int total_cars_finished;
	int num_in_west_left;
	int num_in_west_straight;
	int num_in_west_right;
	int num_in_north_left;
	int num_in_north_straight;
	int num_in_north_right;
	int num_in_south_left;
	int num_in_south_straight;
	int num_in_south_right;
	int num_in_east_left;
	int num_in_east_straight;
	int num_in_east_right;
	int num_out_west_left;
	int num_out_west_straight;
	int num_out_west_right;
	int num_out_north_left;
	int num_out_north_straight;
	int num_out_north_right;
	int num_out_south_left;
	int num_out_south_straight;
	int num_out_south_right;
	int num_out_east_left;
	int num_out_east_straight;
	int num_out_east_right;
} Intersection_State;

static int g_traffic_start_events = 5;

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

void Intersection_EventHandler(Intersection_State *, tw_bf *, Msg_Data *, tw_lp *);
void Intersection_RC_EventHandler(Intersection_State *, tw_bf *, Msg_Data *, tw_lp *);
void Intersection_Statistics_CollectStats(Intersection_State *, tw_lp *);
void  Intersection_StartUp(Intersection_State *SV, tw_lp * lp); 

#endif
