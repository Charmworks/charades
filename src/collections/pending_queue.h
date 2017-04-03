// Generic interface for a pending queue.

class PendingQueue {
  public:
    virtual void push(Event*) = 0;
    virtual Event* pop() = 0;
    virtual void erase(Event*) = 0;
    virtual Time min() const = 0;
    virtual size_t size() const = 0;
};
