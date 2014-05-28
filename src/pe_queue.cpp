#include "pe_queue.h"

PEQueue::PEQueue() {
  size = 0;
  // TODO: Set this sensibly. Maybe 2x number of LPs per PE or something.
  capacity = 20;
  heap = new LPToken*[capacity];
}

PEQueue::~PEQueue() {
  delete[] heap;
}

// Inserts the token pointer at the bottom of the heap, then pulls it up to
// the correct index in a log number of steps.
void PEQueue::insert(LPToken* t) {
  if (size == capacity) {
    LPToken** tmp = new LPToken*[capacity*2];
    for (int i = 0; i < capacity; i++) {
      tmp[i] = heap[i];
    }
    delete[] heap;
    heap = tmp;
    capacity = capacity*2;
  }
  heap[size] = t;
  heap[size]->index = size;
  pull_up(size);
  size++;
}

// Decrement size, swap the token with a leaf, then push the leaf back down in a
// log number of steps.
void PEQueue::remove(LPToken* t) {
  unsigned index = t->index;
  size--;
  swap(index, size);
  push_down(index);
}

// Update the timestamp of the token, and then if necessary move the token.
void PEQueue::update(LPToken* t, Time ts) {
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
void PEQueue::pull_up(unsigned index) {
  unsigned new_index;
  while (has_parent(index)) {
    new_index = smallest(index, parent(index));
    if (new_index != index) {
      swap(new_index, index);
      index = new_index;
    } else {
      break;
    }
  }
}

// Push the token at index down to the correct place in the heap, and update
// the index fields of all affected tokens.
void PEQueue::push_down(unsigned index) {
  unsigned new_index;
  while (has_left(index)) {
    new_index = smallest(index, left(index));
    if (has_right(index)) {
      new_index = smallest(new_index, right(index));
    }
    if (new_index != index) {
      swap(new_index, index);
      index = new_index;
    } else {
      break;
    }
  }
}

unsigned PEQueue::smallest(unsigned i1, unsigned i2) const {
  if (heap[i1]->ts < heap[i2]->ts) {
    return i1;
  } else {
    return i2;
  }
}

bool PEQueue::has_parent(unsigned index) const {
  return index > 0;
}

bool PEQueue::has_left(unsigned index) const {
  return left(index) < size;
}

bool PEQueue::has_right(unsigned index) const {
  return right(index) < size;
}

unsigned PEQueue::parent(unsigned index) const {
  return (index - 1) / 2;
}

unsigned PEQueue::left(unsigned index) const {
  return index * 2 + 1;
}

unsigned PEQueue::right(unsigned index) const {
  return index * 2 + 2;
}
