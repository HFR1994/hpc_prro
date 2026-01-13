#ifndef RRA_PARALLEL_WRAPPERS_H
#define RRA_PARALLEL_WRAPPERS_H
#include "handlers.h"
#include "registers.h"

leader_t set_global_leader(prro_state_t *local, prra_cfg_t global, const mpi_ctx_t *ctx);

#endif //RRA_PARALLEL_WRAPPERS_H