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

/**
 * \brief Convert a string to a boolean value
 * \param s String to convert
 * \param default_value Default value if string is NULL or empty
 * \return Boolean value as int (0 or 1)
 */
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

metadata_state_t get_bounds(prra_cfg_t global, const mpi_ctx_t * ctx) {

    metadata_state_t metadata;

    const int rows_per_rank = global.pop_size / ctx->size;
    const int remainder = global.pop_size % ctx->size;

    metadata.local_rows = rows_per_rank + (ctx->rank < remainder);
    metadata.start_row = ctx->rank * rows_per_rank + (ctx->rank < remainder ? ctx->rank : remainder);

    return metadata;
}
