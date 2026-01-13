#include "mpi/wrappers.h"

#include <stdlib.h>

#include "mpi/registers.h"

#include "utils/global.h"
#include "utils/logger.h"
#include "utils/objective_function.h"

leader_t set_global_leader(prro_state_t *local, const prra_cfg_t global, const mpi_ctx_t *ctx) {

    leader_t l_leader, g_leader;

    int current_leader = set_leader(local, global);
    log_debug("Current leader is %d with %f", current_leader, local->fitness[current_leader]);

    l_leader.fitness = local->best_fitness;
    l_leader.rank = ctx->rank;
    l_leader.index = local->best_idx;
    l_leader.location = local->leader;

    MPI_CHECK(MPI_Allreduce(
        &l_leader,
        &g_leader,
        1,
        MPI_LEADER,
        MPI_LEADER_MIN,
        ctx->comm
    ));

    MPI_Bcast(l_leader.location, global.features, MPI_DOUBLE, g_leader.rank, ctx->comm);

    mpi_assert_vector(l_leader.location, global.features, ctx);

    int leader_location = 0;
    if (ctx->rank == 0) {
        metadata_state_t metadata = get_bounds(global, ctx->size, ctx->rank);
        leader_location = metadata.start_row + g_leader.index;
    }

    MPI_CHECK(MPI_Bcast(&leader_location, 1, MPI_INT, 0, ctx->comm));
    g_leader.index = leader_location;
    g_leader.location = l_leader.location;


    return g_leader;
}
