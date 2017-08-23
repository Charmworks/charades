#ifndef INTERSECTION_H_
#define INTERSECTION_H_

#include "traffic.h"

struct ArrivalMsg {
  Car car;
};

struct DepartureMsg {
  Car car;
};

struct DirectionMsg {
  Car car;
};

class IntersectionLP : public LP<IntersectionLP,
                                 ArrivalMsg,
                                 DepartureMsg,
                                 DirectionMsg> {
private:
  uint32_t this_x, this_y;
  uint32_t initial_cars, total_cars_arrived, total_cars_finished;
  uint32_t in_lane[4], out_lane[4];
  uint64_t neighbors[4];
public:
  IntersectionLP(uint32_t cars);
  void initialize();
  void finalize();

  void forward(ArrivalMsg* msg, tw_bf* bf);
  void reverse(ArrivalMsg* msg, tw_bf* bf);
  void commit(ArrivalMsg* msg, tw_bf* bf);

  void forward(DepartureMsg* msg, tw_bf* bf);
  void reverse(DepartureMsg* msg, tw_bf* bf);
  void commit(DepartureMsg* msg, tw_bf* bf);

  void forward(DirectionMsg* msg, tw_bf* bf);
  void reverse(DirectionMsg* msg, tw_bf* bf);
  void commit(DirectionMsg* msg, tw_bf* bf);
};

#endif
