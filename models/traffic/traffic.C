#include "traffic.h"

#include "intersection.h"

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

int main(int argc, char * argv[]) {
  ArgumentSet args("TRAFFIC Model");
  args.register_argument("mean", "Exponential distribution mean for timestamps", mean);
  args.register_argument("num-x", "Width of the block of intersections", g_num_x);
  args.register_argument("num-y", "Height of the block of intersections", g_num_y);
  args.register_argument("total-cars", "Total number of cars being simulated", g_total_cars);
  args.register_argument("percent-start", "Pecent of cars that start in clustered block [0-1]", g_percent_start);
  args.register_argument("start-size", "Size of start cluster block. X by X block", g_start_size);
  args.register_argument("start-x", "X coord of upper left corner of start cluster", g_start_x);
  args.register_argument("start-y", "Y coord of upper left corner of start cluster", g_start_y);
  args.register_argument("percent-end", "Pecent of cars that end in clustered block [0-1]", g_percent_end);
  args.register_argument("end-size", "Size of end cluster block. X by X block", g_end_size);
  args.register_argument("end-x", "X coord of upper left corner of end cluster", g_end_x);
  args.register_argument("end-y", "Y coord of upper left corner of end cluster", g_end_y);
  tw_add_arguments(&args);

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
