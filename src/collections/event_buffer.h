#ifndef EVENT_BUFFER_H
#define EVENT_BUFFER_H

#include "event.h"
#include "statistics.h"
#include "util.h"

#include <stdio.h>      // Included for size_t

class Event;
class RemoteEvent;

class EventBuffer {
  private:
    size_t msg_size;
    unsigned max_events;
    unsigned max_remote_events;
    unsigned stack_pointer;
    unsigned remote_stack_pointer;
    
    Event* abort_event;
    Event** buffer;
    RemoteEvent** remote_buffer;
  public:
    EventBuffer(unsigned max, unsigned max_remote, size_t sz) :
        max_events(max), max_remote_events(max_remote), msg_size(sz),
        stack_pointer(max), remote_stack_pointer(max_remote) {
      abort_event = new Event;
      abort_event->eventMsg = new (msg_size) RemoteEvent;
      int err = posix_memalign((void **)&buffer, 64, max*sizeof(Event*));
      err = posix_memalign((void **)&remote_buffer, 64, max_remote*sizeof(RemoteEvent*));

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
      for (int i = 0; i < remote_stack_pointer; i++) {
        remote_buffer[i] = new (msg_size) RemoteEvent;
      }
    }

    Event* get_abort_event() const {
      return abort_event;
    }

    Event* get_event() {
      TW_ASSERT(stack_pointer > 0, "Out of events to allocate!\n");
      buffer[--stack_pointer]->clear();
      if(max_events - stack_pointer > PE_STATS(max_events_used))
        PE_STATS(max_events_used) = max_events - stack_pointer;
      return buffer[stack_pointer];
    }
    void free_event(Event* e) {
      TW_ASSERT(stack_pointer < max_events, "Freeing event to full buffer\n");
      if (e->eventMsg) {
        free_remote_event(e->eventMsg);
      }
      buffer[stack_pointer++] = e;
    }


    RemoteEvent* get_remote_event() {
      if (remote_stack_pointer > 0) {
        remote_buffer[--remote_stack_pointer]->clear();
        return remote_buffer[remote_stack_pointer];
      } else { 
        PE_STATS(new_event_calls)++;
        return new (msg_size) RemoteEvent;
      }
    }
    void free_remote_event(RemoteEvent* e) {
      if (remote_stack_pointer < max_remote_events) {
        remote_buffer[remote_stack_pointer++] = e;
      } else {
        PE_STATS(del_event_calls)++;
        delete e;
      }
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
