//
// Created by vscode on 1/8/26.
//

#ifndef RRA_SERIAL_NUMBER_GENERATORS_H
#define RRA_SERIAL_NUMBER_GENERATORS_H
#include "pcg_basic.h"
#include <stdbool.h>

double unif_0_1(pcg32_random_t *rng);
double norm_0_1(pcg32_random_t * rng);
double vector_to_distance(double * unit_vector, int features, pcg32_random_t * rng, bool use_normal_distribution);
void gen_unit_vector(double* unit_vector, double norm, int dim);
double unif_interval(pcg32_random_t *rng, double a, double b);

#endif //RRA_SERIAL_NUMBER_GENERATORS_H