#include "factory.h"
#include "intersection.h"
#include "traffic.h"

// Mean time for event delays
Time mean = 1000;

// Number of intersections and cars in the simulation
uint32_t g_num_x = 4;
uint32_t g_num_y = 4;
uint32_t g_total_cars = 256;

// Configuration for congestion at a particular source region
double   g_percent_start = 0.0;
uint32_t g_start_size = 1;
uint32_t g_start_x = 0;
uint32_t g_start_y = 0;

// Configuration for congestion at a particular destination region
double   g_percent_end = 0.0;
uint32_t g_end_size = 1;
uint32_t g_end_x = 0;
uint32_t g_end_y = 0;

const tw_optdef app_opt[] =
{
  TWOPT_GROUP("TRAFFIC Model"),
  TWOPT_UINT("mean", mean, "Exponential distribution mean for timestamps"),
  TWOPT_UINT("num-x", g_num_x, "Width of the block of intersections"),
  TWOPT_UINT("num-y", g_num_y, "Height of the block of intersections"),
  TWOPT_UINT("total-cars", g_total_cars, "Total number of cars being simulated"),
  TWOPT_DOUBLE("percent-start", g_percent_start, "Pecent of cars that start in clustered block [0-1]"),
  TWOPT_UINT("start-size", g_start_size, "Size of start cluster block. X by X block"),
  TWOPT_UINT("start-x", g_start_x, "X coord of upper left corner of start cluster"),
  TWOPT_UINT("start-y", g_start_y, "Y coord of upper left corner of start cluster"),
  TWOPT_DOUBLE("percent-end", g_percent_end, "Pecent of cars that end in clustered block [0-1]"),
  TWOPT_UINT("end-size", g_end_size, "Size of end cluster block. X by X block"),
  TWOPT_UINT("end-x", g_end_x, "X coord of upper left corner of end cluster"),
  TWOPT_UINT("end-y", g_end_y, "Y coord of upper left corner of end cluster"),
  TWOPT_END()
};

int main(int argc, char * argv[]) {
  tw_opt_add(app_opt);
  tw_init(argc, argv);
  if (g_tw_lookahead > mean) {
    CkAbort("Lookahead must be less than mean\n");
  }
  mean = mean - g_tw_lookahead;

  register_msg_type<ArrivalMsg>();
  register_msg_type<DepartureMsg>();
  register_msg_type<DirectionMsg>();

  g_total_lps = g_num_x * g_num_y;

  //reset mean based on lookahead
  if(g_percent_start > 0) {
    tw_create_simulation(new UnbalancedFactory());
  } else {
    tw_create_simulation(new UniformFactory(g_total_lps, g_total_cars));
  }

  if (CkMyPe() == 0) {
    printf("========================================\n");
    printf("Traffice Model Configuration\n");
    printf("Mean...................%llu\n", mean);
    printf("Intersections..........%u x %u\n", g_num_x, g_num_y);
    printf("Total Cars.............%u\n", g_total_cars);
    printf("Start Region Percent...%lf\n", g_percent_start);
    printf("Start Region Location..%u,%u\n", g_start_x, g_start_y);
    printf("Start Region Size......%u x %u\n", g_start_size, g_start_size);
    printf("End Region Percent.....%lf\n", g_percent_end);
    printf("End Region Location....%u,%u\n", g_end_x, g_end_y);
    printf("End Region Size......%u x %u\n", g_end_size, g_end_size);
    printf("========================================\n\n");
  }

  tw_run();
  tw_end();
  return 0;
}
