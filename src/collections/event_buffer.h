#ifndef EVENT_BUFFER_H
#define EVENT_BUFFER_H

#include "ross_event.h"
#include "event.h"

#include <stdio.h> // Included for size_t
#include <assert.h>

class Event;
class RemoteEvent;

class EventBuffer {
  private:
    size_t msg_size;
    unsigned max_events;
    unsigned stack_pointer;
    unsigned remote_stack_pointer;
    Event* abort_event;
    Event** buffer;
    RemoteEvent** remote_buffer;

  public:
    EventBuffer(unsigned max, size_t sz) :
        max_events(max), msg_size(sz),
        stack_pointer(max), remote_stack_pointer(max*0.1) {
      abort_event = new Event;
      abort_event->eventMsg = new (msg_size) RemoteEvent;
      abort_event->userData = abort_event->eventMsg->userData;

      int err = posix_memalign((void **)&buffer, 64, max*sizeof(Event*));
      err = posix_memalign((void **)&remote_buffer, 64, max*sizeof(RemoteEvent*));

      //buffer = new Event*[max];
      Event* temp_buf = new Event[max];

      //int buf_size = 16*8;
      //char *tempbuf;
      //err = posix_memalign((void **)&tempbuf, 64, stack_pointer*buf_size);
      //if(sizeof(Event) > buf_size) {
      //  printf("Buffers messed up, fix me!!\n");
      //}

      for (int i = 0; i < max; i++) {
        buffer[i] = &temp_buf[i];
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
        return buffer[--stack_pointer];
      } else {
        return abort_event;
      }
    }
    void free_event(Event* e) {
      assert(stack_pointer < max_events);
      if (e->eventMsg) {
        free_remote_event(e->eventMsg);
      }
      e->clear();
      buffer[stack_pointer++] = e;
    }


    RemoteEvent* get_remote_event() {
      if (remote_stack_pointer > 0) {
        return remote_buffer[--remote_stack_pointer];
      } else {
        return new (msg_size) RemoteEvent;
      }
    }
    void free_remote_event(RemoteEvent* e) {
      if (remote_stack_pointer < max_events) {
        e->clear();
        remote_buffer[remote_stack_pointer++] = e;
      } else {
        delete e;
      }
    }
};

#endif
