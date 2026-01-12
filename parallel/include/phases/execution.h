#ifndef RRA_SERIAL_EXECUTION_H
#define RRA_SERIAL_EXECUTION_H

#include "utils/pcg_basic.h"

void define_followers(int *followers, int pop_size, int current_leader, int num_followers, pcg32_random_t *rng);
void set_lookout(int features, const double* current_position, double* n_candidate_position, pcg32_random_t *rng, double rPcpt);
void gather_to_roosting(int pop_size, int features, const double * roosting_site, double * current_position);

#endif //RRA_SERIAL_EXECUTION_H