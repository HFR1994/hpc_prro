#ifndef RRA_PARALLEL_GLOBAL_H
#define RRA_PARALLEL_GLOBAL_H
#include <stdbool.h>
#include "mpi/wrapper.h"

/**
 * \brief Check and clamp values to be within the specified bounds
 * \param X Array of values to check
 * \param rows Number of rows to check
 * \param global Global configuration
 */
void check_bounds(double *X, int rows, const prra_cfg_t global);

/**
 * \brief Convert a string to a boolean value
 * \param s String to convert
 * \param default_value Default value if string is NULL or empty
 * \return Boolean value as int (0 or 1)
 */
int str_to_bool(const char *s, bool default_value);

#endif //RRA_PARALLEL_GLOBAL_H
