#include "util.h"

/*#include "charm_functions.h"

#include <charm++.h>  // Included for CkAbort
#include <stdarg.h> // Included for variable length args
#include <stdio.h>

#define tw_abort CkAbort

void tw_printf(const char *file, int line, const char *fmt, ...) {
  va_list	ap;

  va_start(ap, fmt);
  fprintf(stdout, "%s:%i: ", file, line);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  fflush(stdout);
  va_end(ap);
}

void tw_error(const char *file, int line, const char *fmt, ...) {
  va_list	ap;

  va_start(ap, fmt);
  // TODO: C API for CkMyPE?
  fprintf(stdout, "node: %d: error: %s:%i: ", tw_mype(), file, line);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  fflush(stdout);
  fflush(stdout);
  va_end(ap);

  tw_abort("Abort called\n");
}*/
