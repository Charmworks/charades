#ifndef FACTORY_H_
#define FACTORY_H_

#include "intersection.h"
#include "traffic.h"

class UniformFactory : public LPFactory {
  private:
    uint64_t total_intersections, total_cars;
  public:
    UniformFactory(uint64_t i, uint64_t c)
        : total_intersections(i), total_cars(c) {}
    LPBase* create_lp(uint64_t gid) const {
      if (gid < total_cars % total_intersections) {
        return new IntersectionLP(total_cars / total_intersections + 1);
      } else {
        return new IntersectionLP(total_cars / total_intersections);
      }
    }
};

class UnbalancedFactory : public LPFactory {
  private:
    UniformFactory* start_factory;
    UniformFactory* other_factory;
  public:
    UnbalancedFactory() {
      uint32_t start_cars = g_percent_start * g_total_cars;
      uint32_t start_intersections = g_start_size * g_start_size;
      start_factory = new UniformFactory(start_intersections, start_cars);

      uint32_t other_cars = g_total_cars - start_cars;
      uint32_t other_intersections = g_num_x * g_num_y - start_intersections;
      other_factory = new UniformFactory(other_intersections, other_cars);
    }

    LPBase* create_start_lp(uint64_t x, uint64_t y) const {
      uint64_t flat_id = (x - g_start_x) + (y - g_start_y) * g_start_size;
      return start_factory->create_lp(flat_id);
    }

    LPBase* create_other_lp(uint64_t x, uint64_t y) const {
      uint64_t flat_id = x + y * g_num_x;
      if (y >= g_start_y + g_start_size) {
        flat_id -= g_start_size * g_start_size;
      } else if (y >= g_start_y) {
        flat_id -= g_start_size * (y - g_start_y);
        if (x > g_start_x) {
          flat_id -= g_start_size;
        }
      }
      return other_factory->create_lp(flat_id);
    }
      
    LPBase* create_lp(uint64_t gid) const {
      uint64_t lp_x = gid % g_num_x;
      uint64_t lp_y = gid / g_num_x;

      if (lp_x >= g_start_x && lp_x < g_start_x + g_start_size &&
          lp_y >= g_start_y && lp_y < g_start_y + g_start_size) {
        return create_start_lp(lp_x, lp_y);
      } else {
        return create_other_lp(lp_x, lp_y);
      }
    }
};

#endif
