#ifndef RRA_PARALLEL_GLOBAL_H
#define RRA_PARALLEL_GLOBAL_H
#include <stdbool.h>
#include "mpi/handlers.h"

void check_bounds(double *X, int rows, prra_cfg_t global);
int str_to_bool(const char *s, bool default_value);
metadata_state_t get_bounds(prra_cfg_t global, const mpi_ctx_t * ctx);

#endif //RRA_PARALLEL_GLOBAL_H
