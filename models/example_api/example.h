#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#include "ross_api.h"

struct ExampleMessage {
  int payload;  ///< Payload of the event
};

class ExampleLP : public LP<ExampleLP, ExampleMessage> {
  private:
    int neighbor, counter;
  public:
    ExampleLP() {}
    void initialize();
    void forward(ExampleMessage* msg, tw_bf* bf);
    void reverse(ExampleMessage* msg, tw_bf* bf);
    void commit(ExampleMessage* msg, tw_bf* bf);
    void finalize();
};

#endif
