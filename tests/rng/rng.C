#include "rng.h"

template <typename T, typename G>
void DistributionTester<T,G>::test() {
  G rng;
  int index;
  std::vector<T> results(CALLS);
  std::vector<int> counts(CALLS);

  CkPrintf("Testing %s\n", this->name());

  for (index = 0; index < CALLS; index++) {
    counts[index] = rng.count();
    results[index] = call(rng, index);
  }

  CkPrintf("Total RNG count: %i\n", rng.count());

  for (int s = 0; s < MOVES; s++) {
    int target = sequence[s];
    if (index > target) {
      while (rng.count() > counts[target]) {
        rng.prev();
      }
      index = target;
    } else {
      while (index < sequence[s]) {
        assert(counts[index] == rng.count());
        assert(results[index] == call(rng, index));
        index++;
      }
    }
  }
}

int main(int argc, char** argv) {
  IntegerTester<xoroshiro128plus>().test();
  ULongTester<xoroshiro128plus>().test();
  BinomialTester<xoroshiro128plus>().test();
  ExponentialTester<xoroshiro128plus>().test();
  ParetoTester<xoroshiro128plus>().test();
  GammaTester<xoroshiro128plus>().test();
  GeometricTester<xoroshiro128plus>().test();
  Normal01Tester<xoroshiro128plus>().test();
  NormalSDTester<xoroshiro128plus>().test();
  LogNormalTester<xoroshiro128plus>().test();
  PoissonTester<xoroshiro128plus>().test();
  WeibullTester<xoroshiro128plus>().test();

  clcg4::init(31,41);
  IntegerTester<clcg4>().test();
  ULongTester<clcg4>().test();
  BinomialTester<clcg4>().test();
  ExponentialTester<clcg4>().test();
  ParetoTester<clcg4>().test();
  GammaTester<clcg4>().test();
  GeometricTester<clcg4>().test();
  Normal01Tester<clcg4>().test();
  NormalSDTester<clcg4>().test();
  LogNormalTester<clcg4>().test();
  PoissonTester<clcg4>().test();
  WeibullTester<clcg4>().test();

  return 0;
}
