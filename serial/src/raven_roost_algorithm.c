#include "raven_roost_algorithm.h"
#include <stdio.h>
#include <stdlib.h>
#include "phases/initialization.h"

#include "utils/objective_function.h"

double* define_followers(const int pop_size, const int features, );


void RRA(const int pop_size, const int features, const int iterations, const int search_steps_iter,
         const double lower_bound, const double upper_bound, const double radius, const char *dataset_path,
         double *exec_timings, pcg32_random_t *rng) {

    // Allocate memory for population and temporary population
    double *X = malloc(pop_size * features * sizeof(double));
    double *leader = malloc(pop_size * features * sizeof(double));
    double *fitness = malloc(pop_size * features * sizeof(double));
    double *roosting_site = malloc(features * sizeof(double));

    if (!X || !fitness || !roosting_site || !leader) {
        fprintf(stderr, "Memory allocation failed.\n");
        // Clean up already allocated memory and exit gracefully
        free(X);
        free(fitness);
        free(roosting_site);
        free(leader);
        return;
    }

    const double percFollow = 0.2;

    // Initialize all initial calculations return the looking radii
    const double rPcpt = initialize_params(dataset_path, X, fitness, roosting_site, radius,
        pop_size, features, lower_bound, upper_bound, rng);

    // Set leader to the position with the lowest fitness
    int current_leader = set_leader(leader, fitness, X, pop_size, features);




}
