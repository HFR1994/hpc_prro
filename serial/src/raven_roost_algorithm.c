#include "raven_roost_algorithm.h"

#include <math.h>
#include <stdlib.h>

#include "phases/execution.h"
#include "phases/initialization.h"

#include "utils/logger.h"
#include "utils/number_generators.h"
#include "utils/objective_function.h"

void RRA(const int pop_size, const int features, const int iterations, const int search_steps_iter,
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
    double *displacement = malloc(features * sizeof(double));

    // Use to calculate in which direction is the next step
    double *direction = malloc(features * sizeof(double));

    // Make sure always to have more (using ceiling) than percFollow percent
    // Minus one to not count the leader
    const int num_followers = ceil(percFollow * pop_size - 1);
    int *is_follower = malloc(pop_size * sizeof(int));

    if (!food_source || !current_position || !fitness || !roosting_site || !leader || !is_follower || !displacement || !direction) {
        log_err("Memory allocation failed");

        // Clean up already allocated memory and exit gracefully
        free(food_source);
        free(current_position);
        free(fitness);
        free(roosting_site);
        free(leader);
        free(is_follower);
        free(displacement);
        free(direction);
        return;
    }

    // Initialize all initial calculations return the looking radii
    const double rPcpt = initialize_params(dataset_path, food_source, fitness, roosting_site, radius,
        pop_size, features, lower_bound, upper_bound, rng);

    // Set the intial position
    gather_to_roosting(pop_size, features, roosting_site, current_position);

    // Set leader to the position with the lowest fitness
    const int current_leader = set_leader(leader, fitness, food_source, pop_size, features);

    log_debug("Using %d followers out of %d", num_followers, pop_size);

    // Set followers to '1' otherwise '0'
    define_followers(is_follower, pop_size, current_leader, num_followers, rng);

    const double jump_size = 0.2;

    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < pop_size; i++) {

            if (i == current_leader || is_follower[i] == 1) {
                for (int j = 0; j < features; ++j) {
                    direction[i * features + j] = leader[j] - current_position[i * features + j];
                }
            }else {
                for (int j = 0; j < features; ++j) {
                    direction[i * features + j] = food_source[i * features + j] - current_position[i * features + j];
                }
            }

            const double dis_max = vector_to_distance(direction, features, rng, false);
            gen_unit_vector(direction, dis_max, features);

            const double s_max = jump_size * dis_max;

            // Use decay to calculate the distance to move with randomness
            double s_t = s_max * (1.0 - (double)iter / (double)iterations) * unif_0_1(rng);

            for (int j = 0; j < features; ++j) {
                current_position[i * features + j] += s_t * direction[j];
            }
        }
    }


}
