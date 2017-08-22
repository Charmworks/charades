#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#include "charades.h"

struct AddMessage {
  uint64_t value;
};

struct MultMessage {
  uint64_t value;
};

class AdderLP : public LPBase {
  private:
    uint64_t value;
    uint64_t destination;
  public:
    AdderLP(uint64_t v, uint64_t d) : value(v), destination(d) {}
    void initialize() {
      Event* e = tw_event_new<AddMessage>(destination, g_tw_lookahead, this);
      AddMessage* msg = e->get_data<AddMessage>();
      msg->value = value;
      tw_event_send(e);
    }
    void finalize() {}
};

class MultiplierLP : public LPBase {
  private:
    uint64_t value;
    uint64_t destination;
  public:
    MultiplierLP(uint64_t v, uint64_t d) : value(v), destination(d) {}
    void initialize() {
      Event* e = tw_event_new<MultMessage>(destination, g_tw_lookahead, this);
      MultMessage* msg = e->get_data<MultMessage>();
      msg->value = value;
      tw_event_send(e);
    }
    void finalize() {}
};

/**
 * Receives add and multiply messages, and updates our value accordingly.
 * Tie breaking is done by giving a higher priority to one operation.
 */
class ArithmeticLP : public LP<ArithmeticLP, AddMessage, MultMessage> {
  private:
    uint64_t priority;  ///< Type of message to priotize (add or multiply)
    uint64_t expected;  ///< Expected result at the end of the simulation
    uint64_t value;     ///< Current value (starts at identity)
  public:
    ArithmeticLP(uint64_t p, uint64_t e) : priority(p), expected(e) {
      if (priority == get_msg_id<AddMessage>()) {
        value = 0;
      } else {
        value = 1;
      }
    }
    void initialize() {}
    void finalize() {
      CkPrintf("Final value computed is %llu\n", value);
      if (value != expected) {
        CkPrintf("Final Value: %llu, Expected Value: %llu\n", value, expected);
        CkAbort("Arithmetic Error\n");
      }
    }

    void forward(AddMessage* msg, tw_bf* bf) {
      value += msg->value;
    }
    void reverse(AddMessage* msg, tw_bf* bf) {
      value -= msg->value;
    }
    void commit(AddMessage* msg, tw_bf* bf) {}

    void forward(MultMessage* msg, tw_bf* bf) {
      value *= msg->value;
    }
    void reverse(MultMessage* msg, tw_bf* bf) {
      value /= msg->value;
    }
    void commit(MultMessage* msg, tw_bf* bf) {}

    bool compare(const Event* e1, const Event* e2) {
      if (e1->type_id == priority) {
        return true;
      } else {
        return false;
      }
    }
};

/**
 * Configures the test case by determining the number of LPs to create, and
 * the expected results of the simulation to be checked at the end.
 */
class ArithmeticFactory : public LPFactory {
  private:
    uint64_t priority, num_adders, num_multipliers, expected_result;
  public:
    ArithmeticFactory(uint64_t p, uint64_t a, uint64_t m)
        : priority(p), num_adders(a), num_multipliers(m) {
      if (priority == get_msg_id<AddMessage>()) {
        expected_result = 0;
        for (int i = 0; i < num_adders; i++) {
          expected_result += (i+1);
        }
        for (int i = 0; i < num_multipliers; i++) {
          expected_result *= (i+1);
        }
      } else {
        expected_result = 1;
        for (int i = 0; i < num_multipliers; i++) {
          expected_result *= (i+1);
        }
        for (int i = 0; i < num_adders; i++) {
          expected_result += (i+1);
        }
      }
    }

    uint64_t get_total_lps() const {
      return num_adders + num_multipliers + 1;
    }

    uint64_t get_expected_events() const {
      return num_adders + num_multipliers;
    }

    LPBase* create_lp(uint64_t gid) const {
      if (gid == 0) {
        return new ArithmeticLP(priority, expected_result);
      } else if (gid <= num_adders) {
        return new AdderLP(gid, 0);
      } else {
        return new MultiplierLP(gid - num_adders, 0);
      }
    }
};

#endif
