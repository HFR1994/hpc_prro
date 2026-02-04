#include "raven_roost_algorithm.h"

#include <mpi.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "utils/pcg_basic.h"

#include "mpi/handlers.h"
#include "mpi/registers.h"
#include "mpi/wrappers.h"

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

prro_state_t RRA(double *exec_timings, const prra_cfg_t global, pcg32_random_t *rngs, const mpi_ctx_t *ctx) {

    prro_state_t local;

    // Set the struct for MPI and OpenMP
    local_state_init(&local, global, rngs, ctx);

    // Initialize all initial calculations return the looking radii
    // Reusable value for Rleader or rPcpt
    const double rPcpt = initialize_params(global, &local, ctx);

    MPI_CHECK(MPI_Barrier(ctx->comm));
    exec_timings[1] = MPI_Wtime();

    log_main("Looking radii is %f", rPcpt);

    // Set the initial position
    gather_to_roosting(&local, global);

    // Set leader to the position with the lowest fitness
    leader_t current_global_leader = set_global_leader(&local, global, ctx);
    log_main("Current global leader is %d with %f", current_global_leader.index, current_global_leader.fitness);

    // Used for convergence results
    double prev_best_fitness = prev_best_fitness = current_global_leader.fitness;

    // // Set followers to '1' otherwise '0'
    define_followers(&local, global, current_global_leader, ctx);
    log_info("Using %d followers out of %d", local.num_followers, local.local_rows);

    for (int iter = 0; iter < global.iterations; iter++) {
        #pragma omp parallel
        {
            // Thread-private temporary arrays
            double *thread_prev_location = malloc(global.features * sizeof(double));
            double *thread_final_location = malloc(global.features * sizeof(double));
            double *thread_direction = malloc(global.features * sizeof(double));
            double *thread_n_candidate = malloc(global.features * sizeof(double));

            #pragma omp for schedule(dynamic)
            for (int i = 0; i < local.local_rows; i++) {
                // We only want to follower to alternate the "flight path"
                if (local.is_follower[i] == 1) {
                    // Set a nearby location to the leader center position based on hypersphere (N dimensions) radii
                    // Use thread-local temporary storage
                    const double distance = vector_to_distance(thread_n_candidate, global.features, local.rng, true);
                    gen_unit_vector(thread_n_candidate, distance, global.features);
                    const double U = unif_0_1(local.rng);
                    const double r = rPcpt * pow(U, 1.0 / global.features);
                    for (int j = 0; j < global.features; j++) {
                        thread_final_location[j] = local.leader[j] + r * thread_n_candidate[j];
                    }
                } else {
                    // Just copy the food source position
                    memcpy(thread_final_location, local.food_source + i * global.features, global.features * sizeof(double));
                }

                const double initial_to_target = calculate_distance(local.current_position + i * global.features,
                                                              thread_final_location, global.features);
                double path_distance = 0.0;

                for (int step = 0; step < global.flight_steps; ++step) {
                    memcpy(thread_prev_location, local.current_position + i * global.features, global.features * sizeof(double));

                    for (int j = 0; j < global.features; ++j) {
                        thread_direction[j] = thread_final_location[j] - local.current_position[i * global.features + j];
                    }

                    const double dis_max = vector_to_distance(thread_direction, global.features, local.rng, false);
                    gen_unit_vector(thread_direction, dis_max, global.features);

                    // Calculate a random jump between the remaining distance
                    const double remaining_distance = calculate_distance(local.current_position + i * global.features,
                                                                         thread_final_location, global.features);

                    // Random percentage of remaining distance
                    const double s_t = remaining_distance * unif_0_1(local.rng);

                    // Update the new position of the raven
                    // The author only specifies a random step size.
                    // r-i_t = r-i-1_t + d-i_t
                    for (int j = 0; j < global.features; ++j) {
                        local.current_position[i * global.features + j] += s_t * thread_direction[j];
                    }

                    // Check if the new location doesn't fall out of bounds
                    check_bounds(local.current_position + i * global.features, 1, global);

                    // Add the paths
                    path_distance += calculate_distance(thread_prev_location, local.current_position + i * global.features, global.features);

                    for (int lookout = 0; lookout < global.lookout_steps; ++lookout) {
                        // Pass only from the current_position we care about
                        const double distance = vector_to_distance(thread_n_candidate, global.features, local.rng, true);
                        gen_unit_vector(thread_n_candidate, distance, global.features);
                        const double U = unif_0_1(local.rng);
                        const double r = rPcpt * pow(U, 1.0 / global.features);
                        for (int j = 0; j < global.features; j++) {
                            thread_n_candidate[j] = local.current_position[i * global.features + j] + r * thread_n_candidate[j];
                        }

                        // Again check location
                        check_bounds(thread_n_candidate, 1, global);

                        const double n_fitness = objective_function(thread_n_candidate, global);
                        if (n_fitness < local.fitness[i]) {
                            //log_debug("New best fitness found for individual %d at iteration %d", i, iter);
                            memcpy(local.food_source + i * global.features, thread_n_candidate, global.features * sizeof(double));
                            local.fitness[i] = n_fitness;

                            // Early stop, no need to continue looking
                            // We disabled it for speedup measurements to ensure deterministic outputs
                            if (!global.is_measure_speedup && unif_0_1(local.rng) < 0.1) {
                                log_debug("Early stop for individual %d at step %d", i, step);
                                // Outer for
                                step = global.flight_steps;
                                // Inner for
                                break;
                            }
                        }
                    }
                }

                double dest_to_target = calculate_distance(local.current_position + i * global.features, thread_final_location,
                                                           global.features);
                double relative_progress = (initial_to_target - dest_to_target) / initial_to_target;

                log_debug("Raven %d relative progress : %.6f", i, relative_progress);
            }

            free(thread_prev_location);
            free(thread_final_location);
            free(thread_direction);
            free(thread_n_candidate);
        }

        // Evaluate all functions again and designate the leader
        current_global_leader = set_global_leader(&local, global, ctx);

        // // Track convergence data for all ranks
        if (global.convergence_results) {
            local.convergence_results[iter].iteration = iter;
            local.convergence_results[iter].rank = ctx->rank;
            local.convergence_results[iter].fitness = current_global_leader.fitness;
            local.convergence_results[iter].timestamp = MPI_Wtime() - exec_timings[1];
            local.convergence_results[iter].local_best_idx = current_global_leader.index;
            local.convergence_results[iter].improvement = prev_best_fitness - current_global_leader.fitness;
            local.convergence_results[iter].global_best_fitness = prev_best_fitness;
            prev_best_fitness = current_global_leader.fitness;
        }

        // Reshuffle the followers
        define_followers(&local, global, current_global_leader, ctx);

        // Restart from Roosting position
        gather_to_roosting(&local, global);
    }

    log_info("Finished execution, the best is %d with %f", current_global_leader.index, current_global_leader.fitness);

    return local;
}