#include "pending_queue.h"

class PendingSplay : public PendingQueue {
  private:
    void splay(Event*);
  public:
    PendingSplay();
    void push(Event*);
    void erase(Event*);
    Event* pop();

    tw_stime min() const;
    size_t size() const;
};
