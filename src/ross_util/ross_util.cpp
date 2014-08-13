#include "ross_util.h"
#include "../globals.h"

#ifndef NO_GLOBALS
#endif

#ifndef NO_FORWARD_DECLS
#endif


/**
 * Rollback-aware printf, i.e. if the event gets rolled back, undo the printf.
 * We can'd do that of course so we store the message in a buffer until GVT.
 */
int tw_output(tw_lp *lp, const char *fmt, ...) {
  int ret = 0;
  va_list ap;
  tw_event *cev;
  tw_out *temp;

  if (PE_VALUE(g_tw_synchronization_protocol) != OPTIMISTIC) {
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    return 0;
  }

  tw_out *out = allocate_output_buffer();

  cev = lp->owner->currEvent;

  if (cev->out_msgs == NULL) {
    cev->out_msgs = out;
  } else {
    temp = cev->out_msgs;

    while (temp->next != NULL) {
      temp = temp->next;
    }
    temp->next = out;
  }

  va_start(ap, fmt);
  ret = vsnprintf(out->message, sizeof(out->message), fmt, ap);
  va_end(ap);
  if (ret >= 0 && ret < sizeof(out->message)) {
    // Should be successful
  } else {
    tw_printf(TW_LOC, "Message may be too large?");
  }

  return ret;
}

void tw_printf(const char *file, int line, const char *fmt, ...)
{
  va_list	ap;

  va_start(ap, fmt);
  fprintf(stdout, "%s:%i: ", file, line);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  fflush(stdout);
  va_end(ap);
}

void tw_error(const char *file, int line, const char *fmt, ...)
{
  va_list	ap;

  va_start(ap, fmt);
  fprintf(stdout, "node: %ld: error: %s:%i: ", g_tw_mynode, file, line);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  fflush(stdout);
  fflush(stdout);
  va_end(ap);

  CkAbort("Abort called\n");
}
