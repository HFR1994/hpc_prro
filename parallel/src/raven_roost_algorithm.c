#include "raven_roost_algorithm.h"

#include <mpi.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

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

void RRA(const int pop_size, const int features, const int iterations, const int flight_steps, const int lookout_steps,
         const double lower_bound, const double upper_bound, const double radius, const char *dataset_path,
         double *exec_timings, const bool is_measure_speedup, pcg32_random_t *rng) {

    const double percFollow = 0.2;

    // Allocate memory for population and temporary population
    double *food_source = malloc(pop_size * features * sizeof(double));
    double *current_position = malloc(pop_size * features * sizeof(double));
    double *leader = malloc(features * sizeof(double));
    double *fitness = malloc(pop_size * sizeof(double));
    double *roosting_site = malloc(features * sizeof(double));

    double *prev_location = malloc(features * sizeof(double));
    double *final_location = malloc(features * sizeof(double));

    // Store the values of each search for a better food source
    double *n_candidate_position = malloc(features * sizeof(double));

    // Use to calculate in which direction is the next step
    double *direction = malloc(features * sizeof(double));

    // Make sure always to have more (using ceiling) than percFollow percent
    // Minus one to not count the leader
    const int num_followers = ceil(percFollow * pop_size - 1);
    int *is_follower = malloc(pop_size * sizeof(int));

    if (!food_source || !current_position || !fitness || !roosting_site || !leader || !is_follower
        || !n_candidate_position || !direction || !prev_location || !final_location) {
        log_err("Memory allocation failed");

        // Clean up already allocated memory and exit gracefully
        free(food_source);
        free(current_position);
        free(fitness);
        free(roosting_site);
        free(leader);
        free(is_follower);
        free(n_candidate_position);
        free(direction);
        free(prev_location);
        free(final_location);
        return;
    }

    // Initialize all initial calculations return the looking radii
    // Reusable value for Rleader or rPcpt
    const double rPcpt = initialize_params(dataset_path, food_source, fitness, roosting_site, radius,
        pop_size, features, lower_bound, upper_bound, rng);

    MPI_Barrier(MPI_COMM_WORLD);
    exec_timings[1] = MPI_Wtime();

    log_main("Looking radii is %f", rPcpt);

    // Set the initial position
    gather_to_roosting(pop_size, features, roosting_site, current_position);

    // Set leader to the position with the lowest fitness
    int current_leader = set_leader(leader, fitness, food_source, pop_size, features);
    log_info("Current leader is %d with %f", current_leader, fitness[current_leader]);

    log_info("Using %d followers out of %d", num_followers, pop_size);

    // Set followers to '1' otherwise '0'
    define_followers(is_follower, pop_size, current_leader, num_followers, rng);

    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < pop_size; i++) {

            // We only want to follower to alternate the "flight path"
            if (is_follower[i] == 1) {
                // Set a nearby location to the leader center position based on hypersphere (N dimensions) radii
                set_lookout(features, leader, final_location, rng, rPcpt);
            } else {
                // Just copy the food source position
                memcpy(final_location, food_source + i * features, features * sizeof(double));
            }

            const double initial_to_target = calculate_distance(current_position + i * features, final_location, features);
            double path_distance = 0.0;

            for (int step = 0; step < flight_steps; ++step) {

                memcpy(prev_location,current_position + i * features,features * sizeof(double));

                for (int j = 0; j < features; ++j) {
                    direction[j] = final_location[j] - current_position[i * features + j];
                }

                const double dis_max = vector_to_distance(direction, features, rng, false);
                gen_unit_vector(direction, dis_max, features);

                // Calculate a random jump between the remaining distance
                const double remaining_distance = calculate_distance(current_position + i * features,
                    final_location, features);

                // Random percentage of remaining distance
                const double s_t = remaining_distance * unif_0_1(rng);

                // Update the new position of the raven
                // The author only specifies a random step size.
                // r-i_t = r-i-1_t + d-i_t
                for (int j = 0; j < features; ++j) {
                    current_position[i * features + j] += s_t * direction[j];
                }

                // Check if the new location doesn't fall out of bounds
                check_bounds(current_position + i * features, 1, features, lower_bound, upper_bound);

                // Add the paths
                path_distance += calculate_distance(prev_location,current_position + i * features, features);

                for (int lookout = 0; lookout < lookout_steps; ++lookout) {
                    // Pass only from the current_position we care about
                    set_lookout(features, current_position + i * features, n_candidate_position, rng, rPcpt);

                    // Again check location
                    check_bounds(n_candidate_position, 1, features, lower_bound, upper_bound);

                    const double n_fitness = objective_function(n_candidate_position, features);
                    if (n_fitness < fitness[i]) {
                        //log_debug("New best fitness found for individual %d at iteration %d", i, iter);
                        memcpy(food_source + i * features, n_candidate_position, features * sizeof(double));
                        fitness[i] = n_fitness;

                        // Early stop, no need to continue looking
                        // We disabled it for speedup measurements to ensure deterministic outputs
                        if (!is_measure_speedup && unif_0_1(rng) < 0.1) {
                            log_debug("Early stop for individual %d at step %d", i, step);
                            // Outer for
                            step = flight_steps;
                            // Inner for
                            break;
                        }
                    }
                }
            }

            double dest_to_target = calculate_distance(current_position + i * features, final_location, features);
            double relative_progress = (initial_to_target - dest_to_target) / initial_to_target;

            log_debug("Raven %d relative progress : %.6f", i, relative_progress);
        }

        // Evaluate all functions again and designate the leader
        current_leader = set_leader(leader, fitness, food_source, pop_size, features);

        // Reshuffle the followers
        define_followers(is_follower, pop_size, current_leader, num_followers, rng);

        // Restart from Roosting position
        gather_to_roosting(pop_size, features, roosting_site, current_position);
    }

    log_info("Finished execution, the best is %d with %f", current_leader, fitness[current_leader]);

    // Cleanup
    free(food_source);
    free(current_position);
    free(fitness);
    free(roosting_site);
    free(leader);
    free(is_follower);
    free(n_candidate_position);
    free(direction);
    free(prev_location);
    free(final_location);
}
