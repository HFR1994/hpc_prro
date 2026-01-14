#include "phases/initialization.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "utils/dir_file_handler.h"
#include "utils/global.h"
#include "utils/logger.h"
#include "utils/number_generators.h"
#include "utils/objective_function.h"


void local_state_init(prro_state_t * local, const prra_cfg_t global, const mpi_ctx_t * ctx) {

    memset(local, 0, sizeof *local);
    const metadata_state_t metadata = get_bounds(global, ctx->size, ctx->rank);

    local->local_rows = metadata.local_rows;
    local->start_row = metadata.start_row;

    // Allocate memory for population and temporary population
    local->food_source = malloc(local->local_rows * global.features * sizeof(double));
    local->current_position = malloc(local->local_rows * global.features * sizeof(double));
    local->fitness = malloc(local->local_rows * sizeof(double));
    local->roosting_site = malloc(global.features * sizeof(double));
    local->leader = malloc(global.features * sizeof(double));

    local->prev_location = malloc(global.features * sizeof(double));
    local->final_location = malloc(global.features * sizeof(double));

    // Store the values of each search for a better food source
    local->n_candidate_position = malloc(global.features * sizeof(double));

    // Use to calculate in which direction is the next step
    local->direction = malloc(global.features * sizeof(double));

    // Initialize the count to zero because we don't still know how many followers each rank will recieve
    local->num_followers = 0;
    local->is_follower = malloc(local->local_rows * sizeof(int));

    if (!local->food_source || !local->current_position || !local->fitness
        || !local->roosting_site || !local->leader || !local->is_follower
        || !local->n_candidate_position || !local->direction || !local->prev_location
        || !local->final_location) {

        log_err("Memory allocation failed");

        // Clean up already allocated memory and exit gracefully
        free(local->food_source);
        free(local->current_position);
        free(local->fitness);
        free(local->roosting_site);
        free(local->leader);
        free(local->is_follower);
        free(local->n_candidate_position);
        free(local->direction);
        free(local->prev_location);
        free(local->final_location);
        ERR_CLEANUP();
    }
}

/**
 * \brief Initialization stage
 * \param global Global program configurations
 * \param local Local state for this rank
 * \param ctx MPI context
 * \param rng The random number generator seed
 * \return The radii for the initialization
 */
double initialize_params(const prra_cfg_t global, prro_state_t * local, const mpi_ctx_t * ctx, pcg32_random_t * rng){

    MPI_Request req;

    // Start by computing the roosting, meanwhile do extra work
    if (ctx->rank == 0) {
        // Set roosting site
        for (int i = 0; i < global.features; i++) {
            // Pick a random number between upper and lower bound
            local->roosting_site[i] = unif_interval(rng, global.lower_bound, global.upper_bound);
        }
    }

    MPI_CHECK(MPI_Ibcast(local->roosting_site, global.features, MPI_DOUBLE,0, ctx->comm, &req));

    // Read CSV and map a 1D array to the X variable
    // Represent the best source for
    read_dataset_csv(global.dataset_path, local, global, ctx);

    // Make sure all vector positions are inside the bounds otherwise set the min/max
    check_bounds(local->food_source, local->local_rows, global);

    // Evaluate Griewank function
    for (int i = 0; i < local->local_rows; i++) {
        local->fitness[i] = objective_function(local->food_source + i * global.features, global);
    }

    // Instead of adding more variables to the method, we just compute the value and assign
    const double term = pow(global.pop_size, 1.0 / global.features);
    const double radii = global.radius/(3.6 * term);

    // Wait for broadcast to complete not need to wrap it in MPI_CHECK
    MPI_CHECK(MPI_Wait(&req, MPI_STATUS_IGNORE));

    return radii;
}