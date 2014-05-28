#ifndef PE_QUEUE_H_
#define PE_QUEUE_H_

#include "typedefs.h"

// Each LP Chare will own a token. When an LP registers with a PE it gives a
// pointer to its token to the PE, which gets stored in an array based heap.
// The pointer and Time in the token are self-explanatory. The index is the
// current index of the token in the array. So when an update needs to happen,
// the LP can pass the PE its token and its new timestamp. Then using the index
// the PE can find the token in the heap in constant time, and move it up or
// down in log time.

class LP;

// TODO: Maybe members should be private and the queue be a friend
struct LPToken {
  LP* lp;
  Time ts;
  unsigned index;

  LPToken(LP* lp) : lp(lp) {}
};

// TODO: The big 3? Or disable them?
// Priority queue of LPToken pointers weighted by timestamp.
class PEQueue {
private:
  unsigned capacity, size;
  LPToken** heap;

  void swap(unsigned, unsigned);
  void pull_up(unsigned);
  void push_down(unsigned);
public:
  PEQueue();

  void insert(LPToken*);
  void remove(LPToken*);

  void update(LPToken*, Time);
};

#endif
