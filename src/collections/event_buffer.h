#ifndef EVENT_BUFFER_H
#define EVENT_BUFFER_H

#include <stdio.h> // Included for size_t

class Event;
class RemoteEvent;

class EventBuffer {
  private:
    unsigned max_events;
    size_t msg_size;
    unsigned stack_pointer;
    unsigned remote_stack_pointer;
    Event** buffer;
    RemoteEvent** remote_buffer;

  public:
    EventBuffer(unsigned, size_t);

    Event* get_event();
    void free_event(Event*);

    RemoteEvent* get_remote_event();
    void free_remote_event(RemoteEvent*);
};

#endif
