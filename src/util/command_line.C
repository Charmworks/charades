#include "command_line.h"

#include "globals.h"
#include <fstream>

AggregateParser command_line_parser;

void tw_add_arguments(ArgumentSet* args) {
  command_line_parser.add_parser(args);
}

void parse_command_line(int argc, char** argv) {
  ArgumentSet kernel("Kernel Options");
  kernel.register_argument("sync",                 "Synchronization Protocol - SEQUENTIAL=1, CONSERVATIVE=2, OPTIMISTIC=3", g_tw_synchronization_protocol);
  kernel.register_argument("gvt",                  "GVT Algorithm - SYNCHRONOUS=1, CD=2, PHASED=3, BUCKETED=4", g_tw_gvt_scheme);
  kernel.register_argument("expected-events",      "Number of committed events expected", g_tw_expected_events);
  kernel.register_argument("end-time",             "Virtual end time", g_tw_ts_end);
  kernel.register_argument("lookahead",            "Virtual lookahead", g_tw_lookahead);
  kernel.register_argument("batch",                "Batch size of a scheduler interval", g_tw_mblock);
  kernel.register_argument("gvt-interval",         "Size of each GVT interval (based on gvt trigger)", g_tw_gvt_interval);
  kernel.register_argument("gvt-trigger",          "Type of GVT interval - SCHEDULER INTERVAL=1, VIRTUAL TIME LEASH=2", g_tw_gvt_trigger);
  kernel.register_argument("gvt-phases",           "Number of phases for the phased GVT algorithm", g_tw_gvt_phases);
  kernel.register_argument("gvt-bucket-size",      "Bucket size for bucketed GVT algorithm", g_tw_gvt_bucket_size);
  kernel.register_argument("gvt-async-reduction",  "Use asynchronous reductions in GVT - NO=0, YES=1", g_tw_async_reduction);
  kernel.register_argument("ldb-interval",         "Number of GVT computations between each LB call", g_tw_ldb_interval);
  kernel.register_argument("max-ldb",              "Max number of times to call load balancing", g_tw_max_ldb);
  kernel.register_argument("ldb-metric",           "Metric used for determining LP load during LB", g_tw_ldb_metric);
  kernel.register_argument("ldb-metric-ts-abs",    "Use absolute metric value - NO=0, YES=1", g_tw_metric_ts_abs);
  kernel.register_argument("ldb-metric-invert",    "Invert the LB metrics - NO=0, YES=1", g_tw_metric_invert);
  kernel.register_argument("stat-interval",        "Number of GVT computations between each stat logging", g_tw_stat_interval);
  kernel.register_argument("num-lps",              "Total number of LPs", g_total_lps);
  kernel.register_argument("num-chares",           "Total number of LPChares", g_num_chares);
  kernel.register_argument("lps-per-chare",        "Number of LPs per LPChare", g_lps_per_chare);
  kernel.register_argument("buffer-size",          "Size of pre-allocated event buffer", g_event_buffer_size);
  kernel.register_argument("report-interval",      "Number of GVT computations between each GVT printout", gvt_print_interval);

  tw_add_arguments(&kernel);

  // Search for --help and print arguments if it's there
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == std::string("--help")) {
      if (CkMyPe() == 0) {
        command_line_parser.print();
      }
      CkExit();
      return;
    }
  }

  for (int i = 1; i < argc; i++) {
    std::string argument(argv[i]);
    if (argument.find("--") == 0) {
      argument.erase(0,2);
      if (!command_line_parser.parse(argument)) {
        if (CkMyPe() == 0) {
          CkPrintf("Warning: --%s is an unrecognized command line option\n",
              argument.c_str());
        }
      }
    } else {
      std::ifstream file(argument);
      if (!file.good()) {
        if (CkMyPe() == 0) {
          CkPrintf("Warning: %s is not a valid configuration file\n",
              argument.c_str());
        }
      } else {
        std::string line;
        while(std::getline(file, line)) {
          if (line.find("//") == 0) {
            continue;
          } else if (line.find_first_not_of("\t\r\n ") == std::string::npos) {
            continue;
          } else if (!command_line_parser.parse(line)) {
            if (CkMyPe() == 0) {
              CkPrintf("Warning: %s in file %s is an unrecognized option\n",
                  line.c_str(), argument.c_str());
            }
          }
        }
      }
    }
  }
}
