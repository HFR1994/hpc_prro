#include "utils/global.h"

#include <stdio.h>

#include "utils/logger.h"

void check_bounds(double *X, const int pop_size, const int features, const double lower_bound, const double upper_bound) {
    // Check and correct out-of-bounds values
    for (int i = 0; i < pop_size * features; i++) {
        if (X[i] < lower_bound) {
            log_debug("X[%d] = %f is below lower_bound %f, clamping to lower_bound",
                   i, X[i], lower_bound);
            X[i] = lower_bound;
        } else if (X[i] > upper_bound) {
            log_debug("X[%d] = %f is above upper_bound %f, clamping to upper_bound",
                   i, X[i], upper_bound);
            X[i] = upper_bound;
        }
    }
}
