#ifndef PROCESSED_QUEUE
#define PROCESSED_QUEUE
#include "typedefs.h"
#include "ross_event.h"
#include "event.h"

class ProcessedQueue {
  size_t length;
  Event *head;
  Event *tail;

  Event** temp_event_buffer;

  public:
  ProcessedQueue() : length(0), head(NULL), tail(NULL) { }

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
        e = tw_event_new(0,0,0);
      }
      pup_processed_event(p, e);
      if (p.isUnpacking()) {
        temp_event_buffer[e->seq_num] = e;
        push_back(e);
      } else {
        e->seq_num = i;
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
    e->next = head;
    e->prev = NULL;
    if(head != NULL) {
      head->prev = e;
    }
    head = e;
    if(tail == NULL) {
      tail = e;
    }
    length++;
  }

  Event * front() const {
    return head;
  }

  tw_stime max() const {
    return (head ? head->ts : 0);
  }

  Event * pop_front() {
    Event *e = head;
    head = e->next;
    if(head == NULL) {
      tail = NULL;
    } else {
      head->prev = NULL;
    }
    length--;
    return e;
  }

  void push_back(Event *e) {
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

  tw_stime min() const {
    return (tail ? tail->ts : DBL_MAX);
  }

  Event * pop_back() {
    Event * e = tail;
    tail = e->prev;
    if(tail != NULL) {
      tail->next = NULL;
    } else {
      head = NULL;
    }
    length--;
    return e;
  }

  void erase(Event *e)
  {
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
