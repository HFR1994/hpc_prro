#include "utils/objective_function.h"

#include <limits.h>
#include <math.h>

#include "utils/logger.h"

/**
 * \brief Use the **Griewank function** as an objective function, RRA tend to
 * converge against a location space, so it's better to have multimodal
 * objective functions
 * \param X The raven's current position in N dimensions
 * \param global Global configuration
 * \return The objective function value
 */
double objective_function(const double *X, const prra_cfg_t global) {

    double sum = 0.0;
    double prod = 1.0;

    // Parallelize the loop with reductions for sum and product
    #pragma omp parallel for num_threads(global.max_threads) reduction(+:sum) reduction(*:prod) if(global.features > 100)
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

    // Find the index with the lowest fitness value using parallel reduction
    min_reference_t minp = { local->fitness[0], 0 };

    #pragma omp parallel for reduction(min:minp)
    for (int i = 0; i < local->local_rows; i++) {
        if (local->fitness[i] < minp.value) {
            minp.value = local->fitness[i];
            minp.index = i;
        }
    }

    // Set leader to the position with the lowest fitness
    #pragma omp parallel for num_threads(global.max_threads) if(global.features > 100)
    for (int i = 0; i < global.features; i++) {
        local->leader[i] = local->food_source[minp.index * global.features + i];
    }

    // Update best fitness tracking
    local->best_idx = minp.index;
    local->best_fitness = local->fitness[minp.index];

    return minp.index;
}
