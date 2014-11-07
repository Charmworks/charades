#ifndef EVENT_BUFFER_H
#define EVENT_BUFFER_H

#include "ross_event.h"
#include "event.h"

#include <stdio.h> // Included for size_t
#include <assert.h>

class Event;

class EventBuffer {
  private:
    size_t msg_size;
    unsigned max_events;
    unsigned stack_pointer;
    unsigned remote_stack_pointer;
    Event* abort_event;
    Event** buffer;

  public:
    EventBuffer(unsigned max, size_t sz) :
        max_events(max), msg_size(sz),
        stack_pointer(max), remote_stack_pointer(max*0.1) {
      abort_event = new Event;

      buffer = (Event**)malloc(max*sizeof(Event*));

      for (int i = 0; i < stack_pointer; i++) {
        buffer[i] = new Event;
      }
    }

    Event* get_abort_event() const {
      return abort_event;
    }

    Event* get_event() {
      if (stack_pointer > 0) {
        return buffer[--stack_pointer];
      } else {
        return abort_event;
      }
    }
    void free_event(Event* e) {
      assert(stack_pointer < max_events);
      buffer[stack_pointer++] = e;
    }
};

#endif
