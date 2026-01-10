#ifndef RRA_SERIAL_RAVEN_ROOST_ALGORITHM_H
#define RRA_SERIAL_RAVEN_ROOST_ALGORITHM_H
#include "utils/pcg_basic.h"


void define_followers(int *followers, int pop_size,
                      int current_leader, int num_followers, pcg32_random_t *rng);

void RRA(int pop_size, int features, int iterations, int search_steps_iter,
         double lower_bound, double upper_bound, double radius, const char *dataset_path,
         double *exec_timings, pcg32_random_t *rng);

#endif //RRA_SERIAL_RAVEN_ROOST_ALGORITHM_H