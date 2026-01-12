#ifndef RRA_PARALLEL_RAVEN_ROOST_ALGORITHM_H
#define RRA_PARALLEL_RAVEN_ROOST_ALGORITHM_H
#include <stdbool.h>

#include "mpi/wrapper.h"

#include "utils/pcg_basic.h"

void RRA(int pop_size, int features, int iterations, int flight_steps, int lookout_steps,
         double lower_bound, double upper_bound, double radius, const char *dataset_path,
         double *exec_timings, bool is_measure_speedup, pcg32_random_t *rng, const mpi_ctx_t * ctx);

#endif //RRA_PARALLEL_RAVEN_ROOST_ALGORITHM_H