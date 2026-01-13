#include "mpi/handlers.h"


#include <mpi.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "mpi/registers.h"

#include "utils/logger.h"
#include "utils/number_generators.h"

void mpi_ctx_init(mpi_ctx_t *ctx)
{
    ctx->comm = MPI_COMM_WORLD;

    MPI_CHECK(MPI_Comm_size(ctx->comm, &ctx->size));
    MPI_CHECK(MPI_Comm_rank(ctx->comm, &ctx->rank));
    MPI_Comm_set_errhandler(ctx->comm, MPI_ERRORS_RETURN);

    register_leader_struct();
}

void mpi_ctx_finalize(mpi_ctx_t *ctx)
{
    MPI_Type_free(&MPI_LEADER);
    MPI_Op_free(&MPI_LEADER_MIN);
    (void)ctx; /* currently unused */
}

/**
 * \brief MPI Error Cleanup, use for trackability
 * \param file name of the file where the error is produces
 * \param line line number of error
 * \param func calling function
 * \param mpi_err [Optional] prints MPI error if != MPI_SUCCESS
 * \return
 */
__attribute__((noreturn)) int err_cleanup_impl(const char *file, int line, const char *func, int mpi_err) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    log_err("[FATAL][Rank %d] at %s:%d (%s)\n", rank, file, line, func);

    if (mpi_err != MPI_SUCCESS) {
        char errstr[MPI_MAX_ERROR_STRING];
        int len;
        MPI_Error_string(mpi_err, errstr, &len);
        log_err("MPI error: %s\n", errstr);
        mpi_err = EXIT_FAILURE;
    }

    MPI_Abort(MPI_COMM_WORLD, mpi_err);
    exit(mpi_err);
}

/*
 * Assert that all ranks see the same vector (Euclidean norm),
 * typically used for leader, direction, roosting_site, etc.
 */
void mpi_assert_vector(double *vec, const int features, const mpi_ctx_t *ctx){
    /* 1) Local pointer sanity */
    assert(vec != NULL);
    assert(features > 0);

    /* 2) Compute local Euclidean norm (no randomness) */
    const double local_norm = vector_to_distance(vec, features, NULL, false);

    assert(isfinite(local_norm));

    /* 3) Reduce min/max norm across ranks */
    double min_norm = 0.0, max_norm = 0.0;

    MPI_CHECK(MPI_Allreduce(
        &local_norm, &min_norm, 1, MPI_DOUBLE, MPI_MIN, ctx->comm
    ));
    MPI_CHECK(MPI_Allreduce(
        &local_norm, &max_norm, 1, MPI_DOUBLE, MPI_MAX, ctx->comm
    ));

    /* 4) Compare spread to 12 floating point digits */
    const double eps = 1e-12;
    const double diff = fabs(max_norm - min_norm);

    if (diff > eps) {
        if (ctx->rank == 0) {
            log_err("MPI ASSERT FAILED for vector differs across ranks (|max-min| = %.3e)", diff);
        }
        ERR_CLEANUP();
    }
}
