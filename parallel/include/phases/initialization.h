#ifndef RRA_PARALLEL_INITIALIZATION_H
#define RRA_PARALLEL_INITIALIZATION_H
#include "mpi/handlers.h"

#include "utils/pcg_basic.h"

double initialize_params(const prra_cfg_t global, prro_state_t * local, const mpi_ctx_t * ctx, pcg32_random_t * rng);
void local_state_init(prro_state_t * local, prra_cfg_t global, const mpi_ctx_t * ctx);

#endif //RRA_PARALLEL_INITIALIZATION_H