#include "phases/initialization.h"

#include <math.h>

#include "utils/dir_file_handler.h"
#include "utils/global.h"
#include "utils/number_generators.h"
#include "utils/objective_function.h"

/**
 * \brief Initialization stage
 * \param global Global program configurations
 * \param local Local state for this rank
 * \param ctx MPI context
 * \param rng The random number generator seed
 * \return The radii for the initialization
 */
double initialize_params(const prra_cfg_t global, prro_state_t * local, const mpi_ctx_t * ctx, pcg32_random_t * rng){

    // Read CSV and map a 1D array to the X variable
    // Represent the best source for
    read_dataset_csv(global.dataset_path, local, global, ctx);

    // Make sure all vector positions are inside the bounds otherwise set the min/max
    check_bounds(local->food_source, local->local_rows, global);

    // Evaluate Griewank function
    for (int i = 0; i < local->local_rows; i++) {
        local->fitness[i] = objective_function(local->food_source + i * global.features, global);
    }

    if (ctx->rank == 0) {
        // Set roosting site
        for (int i = 0; i < global.features; i++) {
            // Pick a random number between upper and lower bound
            local->roosting_site[i] = unif_interval(rng, global.lower_bound, global.upper_bound);
        }
    }

    MPI_Bcast(local->roosting_site, global.features, MPI_DOUBLE,0, ctx->comm);

    // Instead of adding more variables to the method, we just compute the value and assign
    const double term = pow(global.pop_size, 1.0 / global.features);
    return global.radius/(3.6 * term);

}