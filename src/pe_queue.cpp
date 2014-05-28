#include "pe_queue.h"

PEQueue::PEQueue() {
  // Set capacity and size
  // Allocate the actual heap array
}

void PEQueue::insert(LPToken* t) {
  // If size == cap, reallocate
  // Insert token at a leaf: O(1)
  // Pull token up the heap: O(log n)
  // Increment size
}

void PEQueue::remove(LPToken* t) {
  // Swap token with a leaf: O(1)
  // Push old leaf back down: O(log n)
  // Decrement size
}

void PEQueue::update(LPToken* t) {
  // Determine whether to push down or pull up: O(1)
  // If a move is necessary, do it: O(log n)
}
