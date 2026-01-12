#include "mpi/wrapper.h"

#include <mpi.h>
#include <stdlib.h>

#include "utils/logger.h"

void mpi_ctx_init(mpi_ctx_t *ctx)
{
    ctx->comm = MPI_COMM_WORLD;

    MPI_CHECK(MPI_Comm_size(ctx->comm, &ctx->size));
    MPI_CHECK(MPI_Comm_rank(ctx->comm, &ctx->rank));
    MPI_Comm_set_errhandler(ctx->comm, MPI_ERRORS_RETURN);
}

void mpi_ctx_finalize(mpi_ctx_t *ctx)
{
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
int err_cleanup_impl(const char *file, int line, const char *func, int mpi_err) {
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