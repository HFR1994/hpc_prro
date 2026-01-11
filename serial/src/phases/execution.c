#include "../../include/phases/execution.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "utils/global.h"
#include "utils/number_generators.h"

/**
 * \brief Send the raven back to the meeting point to start a new journey
 * \param pop_size Total population size
 * \param features Number of features in the raven's position
 * \param roosting_site Meeting point
 * \param current_position The current raven position
 */
void gather_to_roosting(const int pop_size, const int features, const double *roosting_site, double *current_position) {
    // Set each position to the roosting site
    for (int i = 0; i < pop_size; i++) {
        memcpy(&current_position[i * features], roosting_site,features * sizeof(double));
    }
}

/**
 * \brief Define which are the followers, excluding the leader
 * \param followers The raven's followers position in N dimensions
 * \param pop_size Total population size
 * \param current_leader Index of the current leader
 * \param num_followers Number of followers to define
 * \param rng The random number generator seed
 */
void define_followers(int *followers, const int pop_size,
    const int current_leader, const int num_followers, pcg32_random_t *rng) {

    if (pop_size > 1 && num_followers < pop_size) {
        // Initialize the array to 0's
        memset(followers, 0, pop_size * sizeof(int));

        // Create a list of available indices (excluding current_leader)
        int *available = malloc((pop_size - 1) * sizeof(int));
        int count = 0;
        for (int i = 0; i < pop_size; i++) {
            if (i != current_leader) {
                available[count++] = i;
            }
        }

        // Shuffle available indices using Fisher-Yates
        for (int i = count - 1; i > 0; i--) {
            const int j = (int) (unif_0_1(rng) * (i + 1));
            const int temp = available[i];
            available[i] = available[j];
            available[j] = temp;
        }

        // Set first num_followers indices to 1
        for (int i = 0; i < num_followers && i < count; i++) {
            followers[available[i]] = 1;
        }

        free(available);
    }
}

/**
 * \brief Calculate a new lookout position around the current position based on hypersphere (N dimensions) radii
 * \param features Number of features to evaluate
 * \param current_position The N-Dimension current position array for each raven
 * \param n_candidate_position The N-Dimension new position to store in
 * \param rng The random state generator
 * \param rPcpt The radii of look
 */
void set_lookout(const int features, const double* current_position, double* n_candidate_position, pcg32_random_t *rng, const double rPcpt) {

    // Vector needs to be biased to the center (Normal distribution) to
    // have a uniform direction on the hypersphere
    const double distance = vector_to_distance(n_candidate_position, features, rng, true);
    gen_unit_vector(n_candidate_position, distance, features);

    const double U = unif_0_1(rng);
    const double r = rPcpt * pow(U, 1.0 / features);

    // "Makes random perceptions within this hypersphere" (Use Euclidean distance)
    for (int j = 0; j < features; j++) {
        n_candidate_position[j] = current_position[j] + r * n_candidate_position[j];
    }
}
