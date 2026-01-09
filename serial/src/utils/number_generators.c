#include <math.h>
#include "utils/number_generators.h"
#include "utils/pcg_basic.h"

// Helper for generating uniform random number in [0, 1] using Melissa O'Neill PCG generation
double unif_0_1(pcg32_random_t *rng) {
    return ldexp(pcg32_random_r(rng), -32);
}

// Helper for generating uniform random number in [a, b]
double unif_interval(pcg32_random_t *rng, double a, double b) {
    if (b < a) {
        // Swap to ensure a <= b
        const double tmp = a;
        a = b;
        b = tmp;
    }
    return a + (b - a) * unif_0_1(rng);
}

