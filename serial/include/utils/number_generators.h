//
// Created by vscode on 1/8/26.
//

#ifndef RRA_SERIAL_NUMBER_GENERATORS_H
#define RRA_SERIAL_NUMBER_GENERATORS_H
#include "pcg_basic.h"

double unif_0_1(pcg32_random_t *rng);
double unif_interval(pcg32_random_t *rng, double a, double b);

#endif //RRA_SERIAL_NUMBER_GENERATORS_H