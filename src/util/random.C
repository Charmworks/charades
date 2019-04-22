#include "random.h"

#include <charm++.h> // Temporary for CkAbort
#include <math.h>

/*
 * tw_rand_init
 */
void tw_rand_init(uint32_t v, uint32_t w) {
#ifndef USE_XOROSHIRO
  tw_rng_stream::init(v, w);
#endif
}

/*
 * tw_rand_integer
 *
 * For LP # gen, return a uniform rn from low to high
 */
/**
 * @bug Be careful not to pass LONG_MAX into the high variable for the
 * function below.  high + 1 will cause it to overflow.
 */
int64_t tw_rand_integer(tw_rng_stream& g, int64_t low, int64_t high) {
  if (high < low) {
    return 0;
  } else {
    return (low + (int64_t)(tw_rand_unif(g) * (high + 1 - low)));
  }
}

/**
 * @bug Be careful not to pass ULONG_MAX into the high variable for the
 * function below.  high + 1 will cause it to overflow.
 */
uint64_t tw_rand_ulong(tw_rng_stream& g, uint64_t low, uint64_t high) {
  if (high < low) {
      return 0;
  } else {
      return (low + (uint64_t)(tw_rand_unif(g) * (high + 1 - low)));
  }
}

int64_t tw_rand_binomial(tw_rng_stream& g, int64_t N, double P) {
  int64_t sucesses = 0;

  for (int trials = 0; trials < N; trials++) {
    if (tw_rand_unif(g) <= P) {
      sucesses++;
    }
  }

  return sucesses;
}

double tw_rand_exponential(tw_rng_stream& g, double Lambda) {
  return -Lambda * log(tw_rand_unif(g));
}

double tw_rand_pareto(tw_rng_stream& g, double shape, double scale) {
  return scale * 1.0/pow(tw_rand_unif(g), 1/shape);
}

double tw_rand_gamma(tw_rng_stream& g, double shape, double scale) {
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

int64_t tw_rand_geometric(tw_rng_stream& g, double P) {
  assert(P > 0);
  int count = 1;
  while (tw_rand_unif(g) > P) {
    count++;
  }

  return count;
}

// Uses the Box-Muller transform to get a random number from a normal
// distribution from two independent random numbers from a uniform distribution
double tw_rand_normal01(tw_rng_stream& g) {
  return (sqrt(-2.0 * log(tw_rand_unif(g))) * sin(tw_opi * tw_rand_unif(g)));
}

double tw_rand_normal_sd(tw_rng_stream& g, double Mu, double Sd) {
  return Mu + (tw_rand_normal01(g) * Sd);
}

double tw_rand_lognormal(tw_rng_stream& g, double mean, double sd) {
  return exp( mean + sd * tw_rand_normal01(g));
}

int64_t tw_rand_poisson(tw_rng_stream& g, double Lambda) {
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

double tw_rand_weibull(tw_rng_stream& g, double mean, double shape) {
  double scale = mean /  tgamma( ((double)1.0 + (double)1.0/shape));
  return scale * pow(-log( tw_rand_unif(g)), (double)1.0/shape);
}
