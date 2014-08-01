#ifndef ROSS_UTIL_H_
#define ROSS_UTIL_H_

#include "../typedefs.h"

// Typedefs

typedef struct tw_out {
    struct tw_out *next;
    tw_kp *owner;
    /** The actual message content */
    char message[256 - 2*sizeof(void *)];
} tw_out;

// API functions

int tw_output (tw_lp *lp, const char *fmt, ...);
void tw_printf(const char *file, int line, const char *fmt, ...);
void tw_error(const char *file, int line, const char *fmt, ...);
void tw_calloc_stats(size_t *bytes_allocated, size_t *bytes_wasted);
void * tw_calloc(const char *file, int line, const char *for_who, size_t e_sz, size_t n);

#endif
