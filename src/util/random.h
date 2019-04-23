#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <math.h>
#include <random>

#include "clcg4.h"
#include "typedefs.h"
#include "xoroshiro.h"

#define tw_opi 6.28318530718

// TODO: This is mainly here to define RNG for LPs, remove someday
#ifdef USE_XOROSHIRO
typedef xoroshiro128plus RNG;
inline void tw_rand_init(uint32_t v, uint32_t w) {}
#else
typedef clcg4 RNG;
inline void tw_rand_init(uint32_t v, uint32_t w) { clcg4::init(v, w); }
#endif

template <typename G>
inline double tw_rand_unif(G& g) {
  return std::generate_canonical<double, std::numeric_limits<double>::digits>(g);
}

template <>
inline double tw_rand_unif(clcg4& g) {
  return g();
}

// TODO: @bug Be careful not to pass LONG_MAX into the high variable for the
// function below.  high + 1 will cause it to overflow.
template <typename G>
int64_t tw_rand_integer(G& g, int64_t low, int64_t high) {
  if (high < low) {
    return 0;
  } else {
    return (low + (int64_t)(tw_rand_unif(g) * (high + 1 - low)));
  }
}

// TODO: @bug Be careful not to pass ULONG_MAX into the high variable for the
// function below.  high + 1 will cause it to overflow.
template <typename G>
uint64_t tw_rand_ulong(G& g, uint64_t low, uint64_t high) {
  if (high < low) {
      return 0;
  } else {
      return (low + (uint64_t)(tw_rand_unif(g) * (high + 1 - low)));
  }
}

template <typename G>
int64_t tw_rand_binomial(G& g, int64_t N, double P) {
  int64_t sucesses = 0;

  for (int trials = 0; trials < N; trials++) {
    if (tw_rand_unif(g) <= P) {
      sucesses++;
    }
  }

  return sucesses;
}

template <typename G>
int64_t tw_rand_geometric(G& g, double P) {
  assert(P > 0);
  int count = 1;
  while (tw_rand_unif(g) > P) {
    count++;
  }

  return count;
}

template <typename G>
double tw_rand_exponential(G& g, double Lambda) {
  return -Lambda * log(tw_rand_unif(g));
}

template <typename G>
double tw_rand_pareto(G& g, double scale, double shape) {
  return scale * 1.0/pow(tw_rand_unif(g), 1/shape);
}

template <typename G>
double tw_rand_gamma(G& g, double shape, double scale) {
  double a, b, q, phi, d;

  if (shape > 1) {
    a = 1 / sqrt(2 * shape - 1);
    b = shape - log(4);
    q = shape + 1 / a;
    phi = 4.5;
    d = 1 + log(phi);

    while (1) {
      double U_One = tw_rand_unif(g);
      double U_Two = tw_rand_unif(g);
      double V = a * log(U_One / (1 - U_One));
      double Y = shape * exp(V);
      double Z = U_One * U_One * U_Two;
      double W = b + q * V - Y;

      double temp1 = W + d - phi * Z;
      double temp2 = log(Z);

      if (temp1 >= 0 || W >= temp2) {
        return scale * Y;
      }
    }
  } else if (shape == 1) {
    return (tw_rand_exponential(g, scale));
  } else {
    b = (exp(1) + shape) / exp(1);

    while (1) {
      double U_One = tw_rand_unif(g);
      double P = b * U_One;

      if (P <= 1) {
        double Y = pow(P, (1 / shape));
        double U_Two = tw_rand_unif(g);

        if (U_Two <= exp(-Y)) {
          return scale * Y;
        }
      } else {
        double Y = -log((b - P) / shape);
        double U_Two = tw_rand_unif(g);

        if (U_Two <= pow(Y, (shape - 1))) {
          return scale * Y;
        }
      }
    }
  }
}

// Uses the Box-Muller transform to get a random number from a normal
// distribution from two independent random numbers from a uniform distribution
template <typename G>
double tw_rand_normal01(G& g) {
  return (sqrt(-2.0 * log(tw_rand_unif(g))) * sin(tw_opi * tw_rand_unif(g)));
}

template <typename G>
double tw_rand_normal_sd(G& g, double Mu, double Sd) {
  return Mu + (tw_rand_normal01(g) * Sd);
}

template <typename G>
double tw_rand_lognormal(G& g, double mean, double sd) {
  return exp( mean + sd * tw_rand_normal01(g));
}

template <typename G>
int64_t tw_rand_poisson(G& g, double Lambda) {
  double a = exp(-Lambda);
  double b = 1;
  int64_t count = 0;

  b = b * tw_rand_unif(g);
  while (b >= a) {
    b = b * tw_rand_unif(g);
    count++;
  }

  return count;
}

template <typename G>
double tw_rand_weibull(G& g, double mean, double shape) {
  double scale = mean /  tgamma( ((double)1.0 + (double)1.0/shape));
  return scale * pow(-log( tw_rand_unif(g)), (double)1.0/shape);
}

#endif
