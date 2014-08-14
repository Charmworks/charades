#ifndef ROSS_UTIL_H_
#define ROSS_UTIL_H_

#include "../typedefs.h"
#include "../charm/event.h"

// API functions

int tw_output (tw_lp *lp, const char *fmt, ...);
void tw_printf(const char *file, int line, const char *fmt, ...);
void tw_error(const char *file, int line, const char *fmt, ...);

#endif
