#ifndef EVENT_BUFFER_H
#define EVENT_BUFFER_H

#include "event.h"
#include "statistics.h"
#include "util.h"

class EventBuffer {
  private:
    uint32_t max_events;
    uint32_t stack_pointer;
    
    Event* abort_event;
    Event** buffer;
  public:
    EventBuffer(unsigned max) : max_events(max), stack_pointer(max) {
      abort_event = new Event();
      //abort_event->eventMsg = new (msg_size) RemoteEvent;
      int err = posix_memalign((void **)&buffer, 64, max*sizeof(Event*));

      // Calculate the buffer size per event to be a multiple of 8 bytes, large
      // enough to hold an event.
      char *temp_buf;
      int buf_size = 8 * (((sizeof(Event) - 1) / 8) + 1);
      err = posix_memalign((void **)&temp_buf, 64, stack_pointer*buf_size);

      if (buf_size < sizeof(Event)) {
        CkAbort("ERROR: Per-event buffer size broken\n");
      }

      for (int i = 0; i < max; i++) {
        buffer[i] = (Event*)temp_buf;
        temp_buf += buf_size;
      }
    }

    Event* get_abort_event() const {
      return abort_event;
    }

    Event* get_event() {
      TW_ASSERT(stack_pointer > 0, "Out of events to allocate!\n");
      buffer[--stack_pointer]->clear();
      if(max_events - stack_pointer > PE_STATS(max_events_used)) {
        PE_STATS(max_events_used) = max_events - stack_pointer;
      }
      return buffer[stack_pointer];
    }
    void free_event(Event* e) {
      TW_ASSERT(stack_pointer < max_events, "Freeing event to full buffer\n");
      buffer[stack_pointer++] = e;
    }

    unsigned current_size() const {
      return stack_pointer;
    }

    unsigned max_size() const {
      return max_events;
    }

    double percent_used() const {
      return ((double)stack_pointer / max_events);
    }
};

#endif
