#include "pe_queue.h"

PEQueue::PEQueue() {
  // Set capacity and size
  // Allocate the actual heap array
  size = 0;
  // TODO: Set this sensibly. Maybe 2x number of LPs per PE or something.
  capacity = 20;
  heap = new LPToken*[capacity];
}

void PEQueue::insert(LPToken* t) {
  // If size == cap, reallocate
  // Insert token at a leaf: O(1)
  // Pull token up the heap: O(log n)
  // Increment size
  if (size == capacity) {
    // Reallocate and do a linear copy
  }
  heap[size] = t;
  heap[size]->index = size;
  pull_up(size);
  size++;
}

void PEQueue::remove(LPToken* t) {
  // Decrement size
  // Swap token with a leaf: O(1)
  // Push old leaf back down: O(log n)
  unsigned index = t->index;
  size--;
  swap(index, size);
  push_down(index);
}

void PEQueue::update(LPToken* t, Time ts) {
  // Determine whether to push down or pull up: O(1)
  // Move it: O(log n)
  Time old = t->ts;
  t->ts = ts;
  if (ts < old) {
    pull_up(t->index);
  } else {
    push_down(t->index);
  }
}

// Swap positions i1 and i2 in the heap and update their index fields
void PEQueue::swap(unsigned i1, unsigned i2) {
  LPToken* tmp = heap[i1];
  heap[i1] = heap[i2];
  heap[i2] = tmp;
  heap[i1]->index = i1;
  heap[i2]->index = i2;
}

// Pull the token at index up to the correct place in the heap, and update
// the index fields of all affected tokens.
void PEQueue::pull_up(unsigned index) {}

// Push the token at index down to the correct place in the heap, and update
// the index fields of all affected tokens.
void PEQueue::push_down(unsigned index) {}
