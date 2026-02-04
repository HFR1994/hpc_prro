#include <math.h>
#include "utils/number_generators.h"

#include <assert.h>
#include <stdbool.h>

#include "utils/logger.h"
#include "utils/pcg_basic.h"

/**
 * \brief Helper for generating uniform random number in [0, 1] using Melissa O'Neill PCG generation
 * \param rng Random number generator state
 * \return
 */
double unif_0_1(pcg32_random_t *rng) {
    return ldexp(pcg32_random_r(rng), -32);
}

/**
 * \brief Helper for generating Gaussian random number using Box-Muller transform
 * \param rng Random number generator state
 * \return
 */
double norm_0_1(pcg32_random_t *rng) {
    const double u1 = unif_0_1(rng);
    const double u2 = unif_0_1(rng);

    // Use Box–Muller to convert uniform random numbers into a random angle and a random radius.
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

/**
 * \brief Calculates the distance of vector using norm
 * \param unit_vector The vector to calculate distance for
 * \param features Dimension of the vector
 * \param rng Random number generator state
 * \param use_normal_distribution Flag to use normal distribution for components
 * \return Euclidean distance of the vector
 */
double vector_to_distance(double *unit_vector, const int features, pcg32_random_t *rng, const bool use_normal_distribution) {
    double norm = 0.0;

    for (int i = 0; i < features; i++) {
        // Generate Gaussian components if asked for
        if (use_normal_distribution) {
            unit_vector[i] = norm_0_1(rng);
        }

        norm += unit_vector[i] * unit_vector[i];
    }

    norm = sqrt(norm);
    return norm;
}

/**
 * \brief Generate a unit vector based on a distance or normal distribution
 * \param unit_vector Pointer to the vector to store the result
 * \param dim Dimension of the vector
 * \param norm The Euclidean Distance of a vector
 */
void gen_unit_vector(double* unit_vector, const double norm, const int dim) {

    // Normalize to unit length
    if (norm > 0.0) {
        for (int i = 0; i < dim; i++) {
            unit_vector[i] /= norm;
        }
    }
}

/**
 * \brief Helper for generating uniform random number in [a, b]
 * \param rng Random number generator state
 * \param a Lower bound interval
 * \param b Upper bound interval
 */
double unif_interval(pcg32_random_t *rng, double a, double b) {
    if (b < a) {
        // Swap to ensure a <= b
        const double tmp = a;
        a = b;
        b = tmp;
    }
    return a + (b - a) * unif_0_1(rng);
}