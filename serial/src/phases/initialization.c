#include "phases/initialization.h"

#include <math.h>

#include "utils/dir_file_handler.h"
#include "utils/global.h"
#include "utils/number_generators.h"
#include "utils/objective_function.h"

/**
 * \brief Initialization stage
 * \param dataset_path A valid CSV file path
 * \param food_source The raven's food source
 * \param fitness The fitness value for each individual in the population (Initialized to 0)
 * \param roosting_site The initial site for each raven
 * \param radius The looking radius for each raven step
 * \param pop_size Total population size
 * \param features Number of features in the raven's position
 * \param lower_bound The maximum lower bound for a feature
 * \param upper_bound The maximum upper bound for a feature
 * \param rng The random number generator seed
 * \return The radii for the initialization
 */
double initialize_params(const char *dataset_path, double *food_source, double *fitness, double *roosting_site,
                         const double radius, const int pop_size, const int features, const double lower_bound,
                         const double upper_bound, pcg32_random_t *rng) {

    // Read CSV and map a 1D array to the X variable
    // Represent the best source for
    read_dataset_csv(dataset_path, food_source, pop_size, features);

    // Make sure all vector positions are inside the bounds otherwise set the min/max
    check_bounds(food_source, pop_size, features, lower_bound, upper_bound);

    // Evaluate Griewank function
    objective_function(food_source, fitness, pop_size, features);

    // Set roosting site
    for (int i = 0; i < features; i++) {
        // Pick a random number between upper and lower bound
        roosting_site[i] = unif_interval(rng, lower_bound, upper_bound);
    }

    // Instead of adding more variables to the method, we just compute the value and assign
    const double term = features * sqrt(pop_size);
    return radius/(3.6 * term);
}
