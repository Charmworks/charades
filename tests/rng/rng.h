#ifndef RNG_H_
#define RNG_H_

#include "charades.h"

#define CALLS 8192
#define MOVES 11

int sequence[MOVES] = {
                        CALLS/2, CALLS, CALLS/4, CALLS/2, CALLS,
                        CALLS/8, CALLS/2, CALLS/4, CALLS, 0, CALLS
                      };

template <typename T, typename G>
class DistributionTester {
public:
  virtual T call(G& g, int index) const { return 0; }
  virtual const char* name() const = 0;
  void test();
};

template <typename G>
class IntegerTester : public DistributionTester<int64_t, G> {
public:
  int64_t call(G& g, int index) const {
    return tw_rand_integer(g, 0, index);
  }
  const char* name() const { return "Integer Distribution"; }
};

template <typename G>
class ULongTester : public DistributionTester<uint64_t, G> {
public:
  uint64_t call(G& g, int index) const {
    return tw_rand_ulong(g, 0, index);
  }
  const char* name() const { return "Unsigned Long Distribution"; }
};

template <typename G>
class BinomialTester : public DistributionTester<int64_t, G> {
public:
  int64_t call(G& g, int index) const {
    return tw_rand_binomial(g, index, 0.5);
  }
  const char* name() const { return "Binomial Distribution"; }
};

template <typename G>
class ExponentialTester : public DistributionTester<double, G> {
public:
  double call(G& g, int index) const {
    return tw_rand_exponential(g, (double)index / CALLS);
  }
  const char* name() const { return "Exponential Distribution"; }
};

template <typename G>
class ParetoTester : public DistributionTester<double, G> {
public:
  double call(G& g, int index) const {
    return tw_rand_pareto(g, 2.0, (double)index);
  }
  const char* name() const { return "Pareto Distribution"; }
};

template <typename G>
class GammaTester : public DistributionTester<double, G> {
public:
  double call(G& g, int index) const {
    return tw_rand_gamma(g, 2.0, (double)index);
  }
  const char* name() const { return "Gamma Distribution"; }
};

template <typename G>
class GeometricTester : public DistributionTester<int64_t, G> {
public:
  int64_t call(G& g, int index) const {
    return tw_rand_geometric(g, (double)(index + 1) / CALLS);
  }
  const char* name() const { return "Geometric Distribution"; }
};

template <typename G>
class Normal01Tester : public DistributionTester<double, G> {
public:
  double call(G& g, int index) const {
    return tw_rand_normal01(g);
  }
  const char* name() const { return "Normal01 Distribution"; }
};

template <typename G>
class NormalSDTester : public DistributionTester<double, G> {
public:
  double call(G& g, int index) const {
    return tw_rand_normal_sd(g, (double)index / CALLS, 2.0);
  }
  const char* name() const { return "NormalSD Distribution"; }
};

template <typename G>
class LogNormalTester : public DistributionTester<double, G> {
public:
  double call(G& g, int index) const {
    return tw_rand_lognormal(g, (double)index / CALLS, 2.0);
  }
  const char* name() const { return "LogNormal Distribution"; }
};

template <typename G>
class PoissonTester : public DistributionTester<int64_t, G> {
public:
  int64_t call(G& g, int index) const {
    return tw_rand_poisson(g, (double)index / CALLS);
  }
  const char* name() const { return "Poisson Distribution"; }
};

template <typename G>
class WeibullTester : public DistributionTester<double, G> {
public:
  double call(G& g, int index) const {
    return tw_rand_weibull(g, (double)index / CALLS, 2.0);
  }
  const char* name() const { return "Weibull Distribution"; }
};

#endif
