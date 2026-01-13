#ifndef RRA_PARALLEL_INITIALIZATION_H
#define RRA_PARALLEL_INITIALIZATION_H
#include "mpi/wrapper.h"

#include "utils/pcg_basic.h"

double initialize_params(const prra_cfg_t global, prro_state_t * local, const mpi_ctx_t * ctx, pcg32_random_t * rng);

#endif //RRA_PARALLEL_INITIALIZATION_H