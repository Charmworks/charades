#ifndef PROCESSED_QUEUE
#define PROCESSED_QUEUE

#include "event.h"
#include "typedefs.h"
#include "util.h"

class ProcessedQueue {
  size_t length;
  Event *head;
  Event *tail;

  Event** temp_event_buffer;

  public:
  ProcessedQueue() : length(0), head(NULL), tail(NULL) { }
  ~ProcessedQueue() {
    while (length) {
      Event* e = pop_front();
      tw_event_free(e,false);
    }
  }

  virtual void pup(PUP::er &p) {
    p | length;

    int temp_items = length;
    if (p.isUnpacking()) {
      temp_event_buffer = new Event*[temp_items];
      length = 0;
    }

    Event* e = head;
    for (int i = 0; i < temp_items; i++) {
      if (p.isUnpacking()) {
        e = event_alloc();
      }
      e->pup(p);
      if (p.isUnpacking()) {
        temp_event_buffer[e->index] = e;
        push_back(e);
      } else {
        e->index = i;
        e = e->next;
      }
    }
  }

  Event** get_temp_event_buffer() const {
    return temp_event_buffer;
  }

  void delete_temp_event_buffer() {
    delete[] temp_event_buffer;
  }
    
  size_t size() const {
    return length;
  }

  void push_front(Event *e) {
    e->state.owner = TW_rollback_q;
    length++;

    e->next = head;
    e->prev = NULL;
    if(head != NULL) {
      head->prev = e;
    }
    head = e;
    if(tail == NULL) {
      tail = e;
    }
  }

  Event * front() const {
    return head;
  }

  Time max() const {
    return (head ? head->ts : 0);
  }

  Event * pop_front() {
    TW_ASSERT(length > 0, "Popping an empty queue from the front\n");

    Event *e = head;
    head = e->next;
    length--;
    if(head == NULL) {
      tail = NULL;
    } else {
      head->prev = NULL;
    }
    e->state.owner = 0;
    return e;
  }

  void push_back(Event *e) {
    e->state.owner = TW_rollback_q;
    length++;

    e->prev = tail;
    if(tail != NULL) {
      tail->next = e;
    }
    tail = e;
    e->next = NULL;
    if(head == NULL) {
      head = e;
    }
  }

  Event * back() const {
    return tail;
  }

  Time min() const {
    return (tail ? tail->ts : TIME_MAX);
  }

  Event * pop_back() {
    TW_ASSERT(length > 0, "Popping an empty queue from the back\n");

    Event * e = tail;
    tail = e->prev;
    length--;
    if(tail != NULL) {
      tail->next = NULL;
    } else {
      head = NULL;
    }
    e->state.owner = 0;
    return e;
  }

  void erase(Event *e) {
    TW_ASSERT(length > 0, "Attempting to erase from an empty queue\n");
    TW_ASSERT(e->state.owner == TW_rollback_q, "Erasing event we don't own\n");

    e->state.owner = 0;
    length--;
    if(e->prev != NULL) {
      e->prev->next = e->next;
    } else {
      head = e->next;
    }
    if(e->next != NULL) {
      e->next->prev = e->prev;
    } else {
      tail = e->prev;
    }
  }
};
#endif
