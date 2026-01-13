#include "utils/global.h"

#include <ctype.h>
#include <stdlib.h>
#include <mpi.h>
#include <strings.h>

#include "utils/logger.h"

/**
 * \brief Check and clamp values to be within the specified bounds
 * \param X Array of values to check
 * \param rows Number of rows to check
 * \param global Global configuration
 */
void check_bounds(double *X, const int rows, const prra_cfg_t global) {
    // Check and correct out-of-bounds values
    for (int i = 0; i < rows * global.features; i++) {
        if (X[i] < global.lower_bound) {
            log_debug("X[%d] = %f is below lower_bound %f, clamping to lower_bound",
                   i, X[i], global.lower_bound);
            X[i] = global.lower_bound;
        } else if (X[i] > global.upper_bound) {
            log_debug("X[%d] = %f is above upper_bound %f, clamping to upper_bound",
                   i, X[i], global.upper_bound);
            X[i] = global.upper_bound;
        }
    }
}

int str_to_bool(const char *s, const bool default_value) {

    const int default_int_value = default_value ? 1 : 0;

    if (s == NULL || *s == '\0') {
        return default_int_value;
    }

    // Skip leading whitespace
    while (isspace((unsigned char)*s)) s++;

    if (!strcasecmp(s, "1") ||
        !strcasecmp(s, "true") ||
        !strcasecmp(s, "yes") ||
        !strcasecmp(s, "on")) {
        return true;
        }

    if (!strcasecmp(s, "0") ||
        !strcasecmp(s, "false") ||
        !strcasecmp(s, "no") ||
        !strcasecmp(s, "off")) {
        return false;
        }

    // Fallback
    return default_int_value;
}
