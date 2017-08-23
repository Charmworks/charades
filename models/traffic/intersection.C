#include "intersection.h"

IntersectionLP::IntersectionLP(uint32_t cars)
    : initial_cars(cars), total_cars_arrived(0), total_cars_finished(0) {
  for (int i = 0; i < 4; i++) {
    in_lane[i] = out_lane[i] = 0;
  }
}

void IntersectionLP::initialize() {
  this_x = gid % g_num_x;
  this_y = gid / g_num_x;

  neighbors[NORTH] = ((this_y - 1 + g_num_y) % g_num_y) * g_num_x + this_x;
  neighbors[SOUTH] = ((this_y + 1) % g_num_y) * g_num_x + this_x;
  neighbors[EAST] = this_y * g_num_x + ((this_x + 1) % g_num_x);
  neighbors[WEST] = this_y * g_num_x + ((this_x - 1 + g_num_x) % g_num_x);

  for (int i = 0; i < initial_cars; i++) {
    Time offset = g_tw_lookahead + tw_rand_exponential(rng, mean);
    Event* e = tw_event_new<ArrivalMsg>(gid, offset, this);
    ArrivalMsg* msg = e->get_data<ArrivalMsg>();
    msg->car.src = gid;
    msg->car.id = i;
    msg->car.current_lane = tw_rand_integer(rng, 0, 3);

    if (g_percent_end > tw_rand_unif(rng)) {
      msg->car.dest_x = tw_rand_integer(rng, 0, g_end_size-1) + g_end_x;
      msg->car.dest_y = tw_rand_integer(rng, 0, g_end_size-1) + g_end_y;
    } else {
      msg->car.dest_x = tw_rand_integer(rng, 0, g_num_x-1);
      msg->car.dest_y = tw_rand_integer(rng, 0, g_num_y-1);
    }

    tw_event_send(e);
  }
}
void IntersectionLP::finalize() {
#ifdef PRINT
  CkPrintf("Total cars finished at (%i,%i): %i\n", this_x, this_y, total_cars_finished);
#endif
}

void IntersectionLP::forward(ArrivalMsg* msg, tw_bf* bf) {
  if (this_x == msg->car.dest_x && this_y == msg->car.dest_y) {
    total_cars_finished++;
  } else {
    uint8_t new_lane = (msg->car.current_lane + 2) % 4;
    Time offset = g_tw_lookahead + tw_rand_exponential(rng, mean);
    Event* e = tw_event_new<DirectionMsg>(gid, offset, this);
    DirectionMsg* new_msg = e->get_data<DirectionMsg>();
    new_msg->car = msg->car;
    new_msg->car.current_lane = new_lane;
    tw_event_send(e);

    total_cars_arrived++;
    in_lane[new_lane]++;
  }
}

void IntersectionLP::reverse(ArrivalMsg* msg, tw_bf* bf) {
  if (this_x == msg->car.dest_x && this_y == msg->car.dest_y) {
    total_cars_finished--;
  } else {
    uint8_t new_lane = (msg->car.current_lane + 2) % 4;
    tw_rand_reverse_unif(rng);
    total_cars_arrived--;
    in_lane[new_lane]--;
  }
}

void IntersectionLP::commit(ArrivalMsg* msg, tw_bf* bf) {}

void IntersectionLP::forward(DepartureMsg* msg, tw_bf* bf) {
  uint64_t destination = neighbors[msg->car.current_lane];
  Time offset = g_tw_lookahead + tw_rand_exponential(rng, mean);
  Event* e = tw_event_new<ArrivalMsg>(destination, offset, this);
  ArrivalMsg* new_msg = e->get_data<ArrivalMsg>();
  new_msg->car = msg->car;
  tw_event_send(e);

  out_lane[msg->car.current_lane]--;
}

void IntersectionLP::reverse(DepartureMsg* msg, tw_bf* bf) {
  tw_rand_reverse_unif(rng);
  out_lane[msg->car.current_lane]++;
}

void IntersectionLP::commit(DepartureMsg* msg, tw_bf* bf) {}

// TODO: Add lane capacity in
// TODO: Add multiple incoming/outgoing lanes
void IntersectionLP::forward(DirectionMsg* msg, tw_bf* bf) {
  uint8_t new_lane, decision;
  if (this_x != msg->car.dest_x && this_y != msg->car.dest_y) {
    // Decide whether to move horizontal or vertically at random
    decision = tw_rand_integer(rng, 0, 1);
  } else if (this_x != msg->car.dest_x) {
    // Move horizontal
    decision = 0;
  } else if (this_y != msg->car.dest_y) {
    // Move vertical
    decision = 1;
  } else {
    // Error
    CkAbort("Choosing a direction from our destination\n");
  }

  if (decision == 0) {
    bf->c0 = 0;
    if (this_x < msg->car.dest_x) {
      bf->c1 = 0;
      new_lane = EAST;
    } else {
      bf->c1 = 1;
      new_lane = WEST;
    }
  } else {
    bf->c0 = 1;
    if (this_y < msg->car.dest_y) {
      bf->c1 = 0;
      new_lane = SOUTH;
    } else {
      bf->c1 = 1;
      new_lane = NORTH;
    }
  }

  Time offset = g_tw_lookahead + tw_rand_exponential(rng, mean);
  Event* e = tw_event_new<DepartureMsg>(gid, offset, this);
  DepartureMsg* new_msg = e->get_data<DepartureMsg>();
  new_msg->car = msg->car;
  new_msg->car.current_lane = new_lane;
  tw_event_send(e);

  in_lane[msg->car.current_lane]--;
  out_lane[new_lane]++;
}

void IntersectionLP::reverse(DirectionMsg* msg, tw_bf* bf) {
  uint8_t new_lane;
  if (this_x != msg->car.dest_x && this_y != msg->car.dest_y) {
    // The decision needed a random number
    tw_rand_reverse_unif(rng);
  }
  if (bf->c0 == 0) { // Was a horizontal move
    if (bf->c1 == 0) {
      new_lane = EAST;
    } else {
      new_lane = WEST;
    }
  } else { // Was a vertical move
    if (bf->c1 == 0) {
      new_lane = SOUTH;
    } else {
      new_lane = NORTH;
    }
  }
  tw_rand_reverse_unif(rng);
  in_lane[msg->car.current_lane]++;
  out_lane[new_lane]--;
}

void IntersectionLP::commit(DirectionMsg* msg, tw_bf* bf) {}
