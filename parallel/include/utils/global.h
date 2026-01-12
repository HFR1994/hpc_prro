#ifndef RRA_PARALLEL_GLOBAL_H
#define RRA_PARALLEL_GLOBAL_H
#include <stdbool.h>

void check_bounds(double *X, int pop_size, int features, double lower_bound, double upper_bound);
bool str_to_bool(const char *s, bool default_value);

#endif //RRA_PARALLEL_GLOBAL_H
