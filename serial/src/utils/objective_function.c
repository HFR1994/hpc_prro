#include "../../include/utils/objective_function.h"
#include <math.h>

#include "utils/logger.h"

/**
 * \brief Use the **Griewank function** as an objective function, RRA tend to converge against a location space,
 * so it's better to have multimodal objective functions
 * \param X The raven's current position in N dimensions
 * \param features Number of features in the raven's position
 * \return The
 */
double objective_function(const double *X, const int features) {

    double sum = 0.0;
    double prod = 1.0;

    for (int j = 0; j < features; j++) {
        const double raven_feature = X[j];
        sum += (raven_feature * raven_feature) / 4000.0;
        prod *= cos(raven_feature / sqrt(j + 1.0));
    }

    return sum - prod + 1.0;
}

int set_leader(double *leader, const double *fitness, const double *X, const int pop_size, const int features) {
    // Find the index with the lowest fitness value
    int min_index = 0;
    for (int i = 0; i < pop_size; i++) {
        if (fitness[i] < fitness[min_index]) {
            min_index = i;
        }
    }

    log_debug("Current leader is %d", min_index);

    // Set leader to the position with the lowest fitness
    for (int i = 0; i < features; i++) {
        leader[i] = X[min_index * features + i];
    }

    return min_index;
}
