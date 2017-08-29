#include "command_line.h"

#include "globals.h"
#include <fstream>

AggregateParser command_line_parser;

void add_arguments(ArgumentSet* args) {
  command_line_parser.add_parser(args);
}

void parse_command_line(int argc, char** argv) {
  ArgumentSet kernel("Kernel Options");
  kernel.register_arg("sync",                 "Synchronization Protocol - SEQUENTIAL=1, CONSERVATIVE=2, OPTIMISTIC=3", g_tw_synchronization_protocol);
  kernel.register_arg("gvt",                  "GVT Algorithm - SYNCHRONOUS=1, CD=2, PHASED=3, BUCKETED=4", g_tw_gvt_scheme);
  kernel.register_arg("expected-events",      "Number of committed events expected", g_tw_expected_events);
  kernel.register_arg("end-time",             "Virtual end time", g_tw_ts_end);
  kernel.register_arg("lookahead",            "Virtual lookahead", g_tw_lookahead);
  kernel.register_arg("batch",                "Batch size of a scheduler interval", g_tw_mblock);
  kernel.register_arg("gvt-interval",         "Size of each GVT interval (based on gvt trigger)", g_tw_gvt_interval);
  kernel.register_arg("gvt-trigger",          "Type of GVT interval - SCHEDULER INTERVAL=1, VIRTUAL TIME LEASH=2", g_tw_gvt_trigger);
  kernel.register_arg("gvt-phases",           "Number of phases for the phased GVT algorithm", g_tw_gvt_phases);
  kernel.register_arg("gvt-bucket-size",      "Bucket size for bucketed GVT algorithm", g_tw_gvt_bucket_size);
  kernel.register_arg("gvt-async-reduction",  "Use asynchronous reductions in GVT - NO=0, YES=1", g_tw_async_reduction);
  kernel.register_arg("ldb-interval",         "Number of GVT computations between each LB call", g_tw_ldb_interval);
  kernel.register_arg("max-ldb",              "Max number of times to call load balancing", g_tw_max_ldb);
  kernel.register_arg("ldb-metric",           "Metric used for determining LP load during LB", g_tw_ldb_metric);
  kernel.register_arg("ldb-metric-ts-abs",    "Use absolute metric value - NO=0, YES=1", g_tw_metric_ts_abs);
  kernel.register_arg("ldb-metric-invert",    "Invert the LB metrics - NO=0, YES=1", g_tw_metric_invert);
  kernel.register_arg("stat-interval",        "Number of GVT computations between each stat logging", g_tw_stat_interval);
  kernel.register_arg("num-lps",              "Total number of LPs", g_total_lps);
  kernel.register_arg("num-chares",           "Total number of LPChares", g_num_chares);
  kernel.register_arg("lps-per-chare",        "Number of LPs per LPChare", g_lps_per_chare);
  kernel.register_arg("buffer-size",          "Size of pre-allocated event buffer", g_event_buffer_size);
  kernel.register_arg("report-interval",      "Number of GVT computations between each GVT printout", gvt_print_interval);

  add_arguments(&kernel);

  // Search for --help and print arguments if it's there
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == std::string("--help")) {
      command_line_parser.print();
      CkExit();
      return;
    }
  }

  for (int i = 1; i < argc; i++) {
    std::string argument(argv[i]);
    if (argument.find("--") == 0) {
      argument.erase(0,2);
      if (!command_line_parser.parse(argument)) {
        CkPrintf("Warning: --%s is an unrecognized command line option\n",
            argument.c_str());
      }
    } else {
      std::ifstream file(argument);
      if (!file.good()) {
        CkPrintf("Warning: %s is not a valid configuration file\n",
            argument.c_str());
      } else {
        std::string line;
        while(std::getline(file, line)) {
          if (line.find("//") == 0) {
            continue;
          } else if (!command_line_parser.parse(line)) {
            CkPrintf("Warning: %s in file %s is an unrecognized option\n",
                line.c_str(), argument.c_str());
          }
        }
      }
    }
  }
}
