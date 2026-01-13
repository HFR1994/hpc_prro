#ifndef RRA_PARALLEL_RAVEN_ROOST_ALGORITHM_H
#define RRA_PARALLEL_RAVEN_ROOST_ALGORITHM_H

#include "mpi/wrapper.h"
#include "utils/pcg_basic.h"

void RRA(double * exec_timings, const prra_cfg_t global, pcg32_random_t *rng, const mpi_ctx_t * ctx);

#endif //RRA_PARALLEL_RAVEN_ROOST_ALGORITHM_H