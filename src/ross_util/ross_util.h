#ifndef ROSS_UTIL_H_
#define ROSS_UTIL_H_

#include "typedefs.h"
#include "statistics.h"

// API functions

//static tw_stime gvt_print_interval = 0.01;
//static tw_stime percent_complete = 0.0;
//inline void gvt_print(Time gvt);

void show_lld(const char *name, tw_stat v);
void show_2f(const char *name, double v);
void show_1f(const char *name, double v);
void show_4f(const char *name, double v);

void tw_stats_old(Statistics *me);

int tw_output (tw_lp *lp, const char *fmt, ...);
void tw_printf(const char *file, int line, const char *fmt, ...);
void tw_error(const char *file, int line, const char *fmt, ...);

#endif
