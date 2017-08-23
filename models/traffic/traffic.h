#ifndef TRAFFIC_H_
#define TRAFFIC_H_

#include "charades.h"

#define NORTH 0
#define EAST  1
#define SOUTH 2
#define WEST  3

// Mean time for event delays
extern Time mean;

// Number of intersections and cars in the simulation
extern uint32_t g_num_x;
extern uint32_t g_num_y;
extern uint32_t g_total_cars;

// Configuration for congestion at a particular source region
extern double   g_percent_start;
extern uint32_t g_start_size;
extern uint32_t g_start_x;
extern uint32_t g_start_y;

// Configuration for congestion at a particular destination region
extern double   g_percent_end;
extern uint32_t g_end_size;
extern uint32_t g_end_x;
extern uint32_t g_end_y;

struct Car {
  uint32_t src, id;
  uint32_t dest_x, dest_y;
  uint8_t current_lane;
};

#endif
