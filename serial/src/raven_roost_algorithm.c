#include "raven_roost_algorithm.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "phases/execution.h"
#include "phases/initialization.h"

#include "utils/global.h"
#include "utils/logger.h"
#include "utils/number_generators.h"
#include "utils/objective_function.h"

void RRA(const int pop_size, const int features, const int iterations, const int flight_steps, const int lookout_steps,
         const double lower_bound, const double upper_bound, const double radius, const char *dataset_path,
         double *exec_timings, pcg32_random_t *rng) {

    const double percFollow = 0.2;

    // Allocate memory for population and temporary population
    double *food_source = malloc(pop_size * features * sizeof(double));
    double *current_position = malloc(pop_size * features * sizeof(double));
    double *leader = malloc(features * sizeof(double));
    double *fitness = malloc(pop_size * sizeof(double));
    double *roosting_site = malloc(features * sizeof(double));

    // Store the values of each search for a better food source
    double *n_candidate_position = malloc(features * sizeof(double));

    // Use to calculate in which direction is the next step
    double *direction = malloc(features * sizeof(double));

    // Make sure always to have more (using ceiling) than percFollow percent
    // Minus one to not count the leader
    const int num_followers = ceil(percFollow * pop_size - 1);
    int *is_follower = malloc(pop_size * sizeof(int));

    if (!food_source || !current_position || !fitness || !roosting_site || !leader || !is_follower || !n_candidate_position || !direction) {
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
        return;
    }

    // Initialize all initial calculations return the looking radii
    // Reusable value for Rleader or rPcpt
    const double rPcpt = initialize_params(dataset_path, food_source, fitness, roosting_site, radius,
        pop_size, features, lower_bound, upper_bound, rng);

    // Set the initial position
    gather_to_roosting(pop_size, features, roosting_site, current_position);

    // Set leader to the position with the lowest fitness
    int current_leader = set_leader(leader, fitness, food_source, pop_size, features);
    log_debug("Current leader is %d", current_leader);

    log_debug("Using %d followers out of %d", num_followers, pop_size);

    // Set followers to '1' otherwise '0'
    define_followers(is_follower, pop_size, current_leader, num_followers, rng);

    const double jump_size = 0.2;

    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < pop_size; i++) {
            if (is_follower[i] == 1) {
                // Set a nearby location to the leader center position based on hypersphere (N dimensions) radii
                set_lookout(features, leader, n_candidate_position, rng, rPcpt);
            } else {
                // Just copy the food source position
                memcpy(n_candidate_position, food_source + i * features, features * sizeof(double));
            }

            for (int step = 0; step < flight_steps; ++step) {

                // We only want to follower to alternate the "flight path"
                if (i != current_leader && is_follower[i] == 1) {
                    for (int j = 0; j < features; ++j) {
                        direction[j] = n_candidate_position[j] - current_position[i * features + j];
                    }
                }else {
                    for (int j = 0; j < features; ++j) {
                        direction[j] = n_candidate_position[j] - current_position[i * features + j];
                    }
                }

                const double dis_max = vector_to_distance(direction, features, rng, false);
                gen_unit_vector(direction, dis_max, features);

                const double s_max = jump_size * dis_max;

                // Use decay to calculate the distance to move with randomness
                const double s_t = s_max * (1.0 - (double)step / (double)flight_steps) * unif_0_1(rng);

                // Update the new position of the raven
                // The author only specifies a random direction. So added a few elements to increase the search space
                // r-i_t = r-i-1_t + d-i_t
                for (int j = 0; j < features; ++j) {
                    current_position[i * features + j] += s_t * direction[j];
                }

                // Check if the new location doesn't fall out of bounds
                check_bounds(current_position + i * features, 1, features, lower_bound, upper_bound);

                for (int lookout = 0; lookout < lookout_steps; ++lookout) {
                    // Pass only from the current_position we care about
                    set_lookout(features, current_position + i * features, n_candidate_position, rng, rPcpt);

                    // Again check location
                    check_bounds(n_candidate_position, 1, features, lower_bound, upper_bound);

                    const double n_fitness = objective_function(n_candidate_position, features);
                    if (n_fitness < fitness[i]) {
                        log_debug("New best fitness found for individual %d at iteration %d", i, iter);
                        memcpy(food_source + i * features, n_candidate_position, features * sizeof(double));
                        fitness[i] = n_fitness;

                        // Early stop, no need to continue looking
                        if (unif_0_1(rng) < 0.1) {
                            log_debug("Early stop for individual %d at iteration %d", i, iter);
                            // Outer for
                            step = flight_steps;
                            // Inner for
                            break;
                        }
                    }
                }
            }
        }

        // Evaluate all functions again and designate the leader
        current_leader = set_leader(leader, fitness, food_source, pop_size, features);

        // Reshuffle the followers
        define_followers(is_follower, pop_size, current_leader, num_followers, rng);

        // Restart from Roosting position
        gather_to_roosting(pop_size, features, roosting_site, current_position);
    }

    log_info("Finished execution, the best %d with %f", current_leader, fitness[current_leader]);
}
