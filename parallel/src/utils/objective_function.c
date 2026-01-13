#include "../../include/utils/objective_function.h"
#include <math.h>

#include "utils/logger.h"

/**
 * \brief Use the **Griewank function** as an objective function, RRA tend to converge against a location space,
 * so it's better to have multimodal objective functions
 * \param X The raven's current position in N dimensions
 * \param global Global configuration
 * \return The objective function value
 */
double objective_function(const double *X, const prra_cfg_t global) {

    double sum = 0.0;
    double prod = 1.0;

    for (int j = 0; j < global.features; j++) {
        const double raven_feature = X[j];
        sum += (raven_feature * raven_feature) / 4000.0;
        prod *= cos(raven_feature / sqrt(j + 1.0));
    }

    return sum - prod + 1.0;
}

/**
 * \brief Set the leader to the raven with the lowest fitness value
 * \param local Local state for this rank
 * \param global Global configuration
 * \return The index of the leader in the local population
 */
int set_leader(prro_state_t * local, const prra_cfg_t global) {
    // Find the index with the lowest fitness value
    int min_index = 0;
    for (int i = 0; i < local->local_rows; i++) {
        if (local->fitness[i] < local->fitness[min_index]) {
            min_index = i;
        }
    }

    // Set leader to the position with the lowest fitness
    for (int i = 0; i < global.features; i++) {
        local->leader[i] = local->food_source[min_index * global.features + i];
    }

    // Update best fitness tracking
    local->best_idx = min_index;
    local->best_fitness = local->fitness[min_index];

    return min_index;
}
