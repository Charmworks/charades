#ifndef EVENT_BUFFER_H
#define EVENT_BUFFER_H

#include "event.h"
#include "ross_util.h"  // Included for tw_error
#include "statistics.h"

#include <stdio.h>      // Included for size_t
#include <assert.h>

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
      abort_event->userData = abort_event->eventMsg->userData;
      int err = posix_memalign((void **)&buffer, 64, max*sizeof(Event*));
      err = posix_memalign((void **)&remote_buffer, 64, max_remote*sizeof(RemoteEvent*));

      // Calculate the buffer size per event to be a multiple of 8 bytes, large
      // enough to hold an event.
      char *temp_buf;
      int buf_size = 8 * (((sizeof(Event) - 1) / 8) + 1);
      err = posix_memalign((void **)&temp_buf, 64, stack_pointer*buf_size);

      if (buf_size < sizeof(Event)) {
        tw_error(TW_LOC, "ERROR: Per-event buffer size %d, event size %d\n",
            buf_size, sizeof(Event));
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
      if (stack_pointer > 0) {
        buffer[--stack_pointer]->clear();
        if(max_events - stack_pointer > PE_STATS(max_events_used))
          PE_STATS(max_events_used) = max_events - stack_pointer;
        return buffer[stack_pointer];
      } else {
        return abort_event;
      }
    }
    void free_event(Event* e) {
      assert(stack_pointer < max_events);
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
