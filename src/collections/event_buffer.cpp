#include "event_buffer.h"

#include "event.h"
#include "globals.h"
#include "ross_event.h"

#include <assert.h>

EventBuffer::EventBuffer(unsigned max, size_t size) :
    max_events(max), msg_size(size),
    stack_pointer(max), remote_stack_pointer(max*0.1) {
  buffer = (Event**)malloc(max*sizeof(Event*));
  remote_buffer = (RemoteEvent**)malloc(max*sizeof(RemoteEvent*));

  for (int i = 0; i < stack_pointer; i++) {
    buffer[i] = new Event;
  }
  for (int i = 0; i < remote_stack_pointer; i++) {
    remote_buffer[i] = new (msg_size) RemoteEvent;
  }
}

Event* EventBuffer::get_event() {
  if (stack_pointer > 0) {
    return buffer[--stack_pointer];
  } else {
    return PE_VALUE(abort_event);
  }
}

void EventBuffer::free_event(Event* e) {
  assert(stack_pointer < max_events);
  if (e->eventMsg) {
    free_remote_event(e->eventMsg);
    e->eventMsg = NULL;
  }
  buffer[stack_pointer++] = e;
};

RemoteEvent* EventBuffer::get_remote_event() {
  if (remote_stack_pointer > 0) {
    return remote_buffer[--remote_stack_pointer];
  } else {
    return new (msg_size) RemoteEvent;
  }
}

void EventBuffer::free_remote_event(RemoteEvent* e) {
  if (remote_stack_pointer < max_events) {
    remote_buffer[remote_stack_pointer++] = e;
  } else {
    delete e;
  }
}
