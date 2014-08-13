#ifndef PROCESSED_QUEUE
#define PROCESSED_QUEUE
#include "typedefs.h"
#include "event.h"

class ProcessedQueue {
  size_t length;
  Event *head;
  Event *tail;

  public:
  ProcessedQueue() : length(0), head(NULL), tail(NULL) { }

  size_t size() {
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

  Event * front() {
    return head;
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
    if(tail != NULL) {
      tail->next = e;
    }
    tail = e;
    e->next = NULL;
    if(head == NULL) {
      head = e;
    }
  }

  Event * back() {
    return tail;
  }

  Event * pop_back() {
    Event * e = tail;
    tail = e->prev;
    if(tail != NULL) {
      tail->next = NULL;
    } else {
      head = NULL;
    }
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
