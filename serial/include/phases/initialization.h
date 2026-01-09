#ifndef RRA_SERIAL_INITIALIZATION_H
#define RRA_SERIAL_INITIALIZATION_H
#include "utils/pcg_basic.h"

double initialize_params(const char * dataset_path, double * p, double * fitness, double * roosting_site, double radius,
    int pop_size, int features, double lower_bound, double upper_bound, pcg32_random_t * rng);

#endif //RRA_SERIAL_INITIALIZATION_H