#ifndef RRA_PARALLEL_WRAPPER_H
#define RRA_PARALLEL_WRAPPER_H
#include <mpi.h>

/* Small wrapper for MPI state */
typedef struct {
    MPI_Comm comm;
    int rank;
    int size;
} mpi_ctx_t;

int err_cleanup_impl(const char *file, int line, const char *func, int mpi_err);

/* Lifecycle */
void mpi_ctx_init(mpi_ctx_t *ctx);
void mpi_ctx_finalize(mpi_ctx_t *ctx);

/**
 * \brief Macro Call to check an MPI Call error and clean up
 * \param call MPI Call (ej: MPI_CHECK(MPI_Bcast(...)));
 */
#define MPI_CHECK(call) \
do { \
    int _err = (call); \
    if (_err != MPI_SUCCESS) \
        err_cleanup_impl(__FILE__, __LINE__, __func__, _err); \
} while (0)

/**
 * \brief Macro call to clean up in case of error
 */
#define ERR_CLEANUP() err_cleanup_impl(__FILE__, __LINE__, __func__, MPI_SUCCESS);

#endif //RRA_PARALLEL_WRAPPER_H

