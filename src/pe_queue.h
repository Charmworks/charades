#ifndef PE_QUEUE_H_
#define PE_QUEUE_H_

#include "typedefs.h"

class LP;

// Tokens owned by LP chares that are used by the PE queues that control
// scheduling and fossil collection. Each token has a direct pointer to its LP,
// the timestamp associated with the token, and the index of its location in the
// queue.
struct LPToken {
  private:
    LP* lp;
    Time ts;
    unsigned index;

  public:
    LPToken(LP* lp) : lp(lp) {}
    friend class PEQueue;
};

// Priority queue of LPToken pointers weighted by timestamp. The heap is
// maintained as an array of LPToken pointers. The pointers are not owned by the
// queue. The queue must update the index and timestamp entries in the token.
// This allows us to find an entry in the heap in constant time when given a
// pointer to an LPToken, allowing us to update the token with a new timestamp
// and reposition it in the queue accordingly.
class PEQueue {
  private:
    unsigned capacity, size;
    LPToken** heap;

    // Swaps the queue entries at the two indices and updates their index fields.
    void swap(unsigned, unsigned);

    // Repositions entries located at the given index.
    void pull_up(unsigned);
    void push_down(unsigned);

    // Compares the entries at the given indices, and returns the index of the
    // smallest entry.
    unsigned smallest(unsigned, unsigned) const;

    // Helper methods for accessing parents and children based on indices.
    bool has_parent(unsigned) const;
    bool has_left(unsigned) const;
    bool has_right(unsigned) const;
    unsigned parent(unsigned) const;
    unsigned left(unsigned) const;
    unsigned right(unsigned) const;
  public:
    PEQueue();
    ~PEQueue();

    // Returns the token on top of the queue.
    // TODO: We may also want to be able to see the second token.
    LPToken* top() const;

    // Inserts and removes tokens from the queue.
    void insert(LPToken*, Time);
    void remove(LPToken*);

    // Updates the given token with a new timestamp, and repositions it.
    void update(LPToken*, Time);
};

#endif
