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

// Priority queue of LPToken pointers weighted by timestamp.
class PEQueue {
private:
  unsigned capacity, size;
  LPToken** heap;

  void swap(unsigned, unsigned);
  void pull_up(unsigned);
  void push_down(unsigned);

  unsigned smallest(unsigned, unsigned) const;
  bool has_parent(unsigned) const;
  bool has_left(unsigned) const;
  bool has_right(unsigned) const;
  unsigned parent(unsigned) const;
  unsigned left(unsigned) const;
  unsigned right(unsigned) const;
public:
  PEQueue();
  ~PEQueue();

  void insert(LPToken*, Time);
  void remove(LPToken*);

  void update(LPToken*, Time);
};

#endif
