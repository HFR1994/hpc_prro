#include "../../include/phases/execution.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "utils/global.h"
#include "utils/number_generators.h"

/**
 * \brief Send the raven back to the meeting point to start a new journey
 * \param local Local state for this rank
 * \param global Global configuration
 */
void gather_to_roosting(prro_state_t * local, const prra_cfg_t global) {
    // Set each position to the roosting site
    for (int i = 0; i < local->local_rows; i++) {
        memcpy(&local->current_position[i * global.features], local->roosting_site, global.features * sizeof(double));
    }
}

/**
 * \brief Define which are the followers, excluding the leader
 * \param local Local state for this rank
 * \param current_leader Index of the current leader
 * \param rng The random number generator
 */
void define_followers(prro_state_t * local, const int current_leader, pcg32_random_t *rng) {

    if (local->local_rows > 1 && local->num_followers < local->local_rows) {
        // Initialize the array to 0's
        memset(local->is_follower, 0, local->local_rows * sizeof(int));

        // Create a list of available indices (excluding current_leader)
        int *available = malloc((local->local_rows - 1) * sizeof(int));
        int count = 0;
        for (int i = 0; i < local->local_rows; i++) {
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
        for (int i = 0; i < local->num_followers && i < count; i++) {
            local->is_follower[available[i]] = 1;
        }

        free(available);
    }
}

/**
 * \brief Calculate a new lookout position around the current position based on hypersphere (N dimensions) radii
 * \param local Local state for this rank
 * \param global Global configuration
 * \param current_position The N-Dimension current position array for each raven
 * \param rPcpt The radii of look
 * \param rng The random state generator
 */
void set_lookout(prro_state_t * local, const prra_cfg_t global, const double* current_position, const double rPcpt, pcg32_random_t *rng) {

    // Vector needs to be biased to the center (Normal distribution) to
    // have a uniform direction on the hypersphere
    const double distance = vector_to_distance(local->n_candidate_position, global.features, rng, true);
    gen_unit_vector(local->n_candidate_position, distance, global.features);

    const double U = unif_0_1(rng);
    const double r = rPcpt * pow(U, 1.0 / global.features);

    // "Makes random perceptions within this hypersphere" (Use Euclidean distance)
    for (int j = 0; j < global.features; j++) {
        local->n_candidate_position[j] = current_position[j] + r * local->n_candidate_position[j];
    }
}
