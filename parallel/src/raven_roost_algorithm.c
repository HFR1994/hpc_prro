#include "raven_roost_algorithm.h"

#include <mpi.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "mpi/wrapper.h"

#include "phases/execution.h"
#include "phases/initialization.h"

#include "utils/global.h"
#include "utils/logger.h"
#include "utils/number_generators.h"
#include "utils/objective_function.h"

double calculate_distance(const double *initial, const double *finish, const int features) {
    double dist2 = 0.0;

    for (int j = 0; j < features; j++) {
        const double d = finish[j] - initial[j];
        dist2 += d * d;
    }

    return sqrt(dist2);
}

void local_state_init(prro_state_t * local, const prra_cfg_t global, const mpi_ctx_t * ctx) {
    
    const int rows_per_rank = global.pop_size / ctx->size;
    const int remainder = global.pop_size % ctx->size;

    local->local_rows = rows_per_rank + (ctx->rank < remainder);
    local->start_row = ctx->rank * rows_per_rank + (ctx->rank < remainder
        ? ctx->rank : remainder);

    // Allocate memory for population and temporary population
    local->food_source = malloc(local->local_rows * global.features * sizeof(double));
    local->current_position = malloc(local->local_rows * global.features * sizeof(double));
    local->leader = malloc(global.features * sizeof(double));
    local->fitness = malloc(local->local_rows * sizeof(double));
    local->roosting_site = malloc(global.features * sizeof(double));

    local->prev_location = malloc(global.features * sizeof(double));
    local->final_location = malloc(global.features * sizeof(double));

    // Store the values of each search for a better food source
    local->n_candidate_position = malloc(global.features * sizeof(double));

    // Use to calculate in which direction is the next step
    local->direction = malloc(global.features * sizeof(double));

    // Make sure always to have more (using ceiling) than percFollow percent
    // Minus one to not count the leader
    const double percFollow = 0.2;
    local->num_followers = ceil(percFollow * local->local_rows - 1);
    local->is_follower = malloc(global.pop_size * sizeof(int));

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
        ERR_CLEANUP()
    }
}

void RRA(double * exec_timings, const prra_cfg_t global, pcg32_random_t *rng, const mpi_ctx_t * ctx) {

    prro_state_t local;

    local_state_init(&local, global, ctx);

    // Initialize all initial calculations return the looking radii
    // Reusable value for Rleader or rPcpt
    const double rPcpt = initialize_params(global, &local, ctx, rng);

    MPI_CHECK(MPI_Barrier(ctx->comm));
    exec_timings[1] = MPI_Wtime();

    log_main("Looking radii is %f", rPcpt);

    // Set the initial position
    gather_to_roosting(local.local_rows, global.features, local.roosting_site, local.current_position);

    // Set leader to the position with the lowest fitness
    int current_leader = set_leader(local.leader, local.fitness, local.food_source, local.local_rows, global.features);
    log_info("Current leader is %d with %f", current_leader, local.fitness[current_leader]);

    log_info("Using %d followers out of %d", local.num_followers, local.local_rows);

    // Set followers to '1' otherwise '0'
    define_followers(local.is_follower, local.local_rows, current_leader, local.num_followers, rng);

    for (int iter = 0; iter < global.iterations; iter++) {
        for (int i = 0; i < local.local_rows; i++) {
            // We only want to follower to alternate the "flight path"
            if (local.is_follower[i] == 1) {
                // Set a nearby location to the leader center position based on hypersphere (N dimensions) radii
                set_lookout(global.features, local.leader, local.final_location, rng, rPcpt);
            } else {
                // Just copy the food source position
                memcpy(local.final_location, local.food_source + i * global.features, global.features * sizeof(double));
            }

            const double initial_to_target = calculate_distance(local.current_position + i * global.features,
                                                          local.final_location, global.features);
            double path_distance = 0.0;

            for (int step = 0; step < global.flight_steps; ++step) {
                memcpy(local.prev_location, local.current_position + i * global.features, global.features * sizeof(double));

                for (int j = 0; j < global.features; ++j) {
                    local.direction[j] = local.final_location[j] - local.current_position[i * global.features + j];
                }

                const double dis_max = vector_to_distance(local.direction, global.features, rng, false);
                gen_unit_vector(local.direction, dis_max, global.features);

                // Calculate a random jump between the remaining distance
                const double remaining_distance = calculate_distance(local.current_position + i * global.features,
                                                                     local.final_location, global.features);

                // Random percentage of remaining distance
                const double s_t = remaining_distance * unif_0_1(rng);

                // Update the new position of the raven
                // The author only specifies a random step size.
                // r-i_t = r-i-1_t + d-i_t
                for (int j = 0; j < global.features; ++j) {
                    local.current_position[i * global.features + j] += s_t * local.direction[j];
                }

                // Check if the new location doesn't fall out of bounds
                check_bounds(local.current_position + i * global.features, 1, global.features, global.lower_bound,
                             global.upper_bound);

                // Add the paths
                path_distance += calculate_distance(local.prev_location, local.current_position + i * global.features, global.features);

                for (int lookout = 0; lookout < global.lookout_steps; ++lookout) {
                    // Pass only from the current_position we care about
                    set_lookout(global.features, local.current_position + i * global.features, local.n_candidate_position, rng,
                                rPcpt);

                    // Again check location
                    check_bounds(local.n_candidate_position, 1, global.features, global.lower_bound, global.upper_bound);

                    const double n_fitness = objective_function(local.n_candidate_position, global.features);
                    if (n_fitness < local.fitness[i]) {
                        //log_debug("New best fitness found for individual %d at iteration %d", i, iter);
                        memcpy(local.food_source + i * global.features, local.n_candidate_position, global.features * sizeof(double));
                        local.fitness[i] = n_fitness;

                        // Early stop, no need to continue looking
                        // We disabled it for speedup measurements to ensure deterministic outputs
                        if (!global.is_measure_speedup && unif_0_1(rng) < 0.1) {
                            log_debug("Early stop for individual %d at step %d", i, step);
                            // Outer for
                            step = global.flight_steps;
                            // Inner for
                            break;
                        }
                    }
                }
            }

            double dest_to_target = calculate_distance(local.current_position + i * global.features, local.final_location,
                                                       global.features);
            double relative_progress = (initial_to_target - dest_to_target) / initial_to_target;

            log_debug("Raven %d relative progress : %.6f", i, relative_progress);
        }

        // Evaluate all functions again and designate the leader
        current_leader = set_leader(local.leader, local.fitness, local.food_source, local.local_rows, global.features);

        // Reshuffle the followers
        define_followers(local.is_follower, local.local_rows, current_leader, local.num_followers, rng);

        // Restart from Roosting position
        gather_to_roosting(local.local_rows, global.features, local.roosting_site, local.current_position);
    }

    log_info("Finished execution, the best is %d with %f", current_leader, local.fitness[current_leader]);

    // Cleanup
    free(local.food_source);
    free(local.current_position);
    free(local.fitness);
    free(local.roosting_site);
    free(local.leader);
    free(local.is_follower);
    free(local.n_candidate_position);
    free(local.direction);
    free(local.prev_location);
    free(local.final_location);
}
