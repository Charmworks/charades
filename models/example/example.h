#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#include "charades.h"

// Declare message types as structs or classes. No inheritance needed.
struct ExampleMessage {
  ExampleMessage(int p) : payload(p) {}
  int payload;
};

struct ExampleMessage2 {
  ExampleMessage2(int p) : payload(p) {}
  int payload;
};

// Declare LPs with public inheritance from LP<...>
// The first template argument is the derived type, for CRTP
// All subsequent types are the types of messages this LP will have to respond
// to. The base class will automatically register message dispatchers for each
// template argument.
class ExampleLP : public LP<ExampleLP, ExampleMessage, ExampleMessage2, void*> {
  private:
    int neighbor, counter;
  public:
    ExampleLP() {}
    void pup(PUP::er& p) {
      LPBase::pup(p);
      p | neighbor;
      p | counter;
    }

    void initialize();
    void finalize();

    void forward(ExampleMessage* msg, tw_bf* bf);
    void reverse(ExampleMessage* msg, tw_bf* bf);
    void commit(ExampleMessage* msg, tw_bf* bf);

    void forward(ExampleMessage2* msg, tw_bf* bf);
    void reverse(ExampleMessage2* msg, tw_bf* bf);
    void commit(ExampleMessage2* msg, tw_bf* bf);

    void forward(void* msg, tw_bf* bf);
    void reverse(void* msg, tw_bf* bf);
    void commit(void* msg, tw_bf* bf);
};

class ExampleLP2 : public LP<ExampleLP2, ExampleMessage, ExampleMessage2> {
  private:
    int neighbor, counter;
  public:
    ExampleLP2() {}
    void pup(PUP::er& p) {
      LPBase::pup(p);
      p | neighbor;
      p | counter;
    }

    void initialize();
    void finalize();

    void forward(ExampleMessage* msg, tw_bf* bf);
    void reverse(ExampleMessage* msg, tw_bf* bf);
    void commit(ExampleMessage* msg, tw_bf* bf);

    void forward(ExampleMessage2* msg, tw_bf* bf);
    void reverse(ExampleMessage2* msg, tw_bf* bf);
    void commit(ExampleMessage2* msg, tw_bf* bf);
};

#endif
