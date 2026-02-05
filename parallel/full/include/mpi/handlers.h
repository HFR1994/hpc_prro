#ifndef RRA_PARALLEL_WRAPPER_H
#define RRA_PARALLEL_WRAPPER_H
#include <mpi.h>

/* Small wrapper for MPI state */
typedef struct {
    MPI_Comm comm;
    int rank;
    int size;
} mpi_ctx_t;

typedef struct {
    int iteration;
    int rank;
    double fitness;
    double timestamp;
    int local_best_idx;
    double global_best_fitness;
    double improvement;
} convergence_point_t;

typedef struct {
    int pop_size;
    int features;
    int iterations;
    int flight_steps;
    int lookout_steps;
    int max_threads;
    double lower_bound;
    double upper_bound;
    double radius;
    int is_measure_speedup;  // use int for MPI portability
    int convergence_results; // use int for MPI portability
    char dataset_path[1024];
    char output_dir[1024];
    char placement[128];
} prra_cfg_t;

typedef struct {
    /* Population owned by this rank */
    int local_rows;
    int start_row;
} metadata_state_t;

typedef struct {
    /* Population owned by this rank */
    int local_rows;
    int start_row;

    double *food_source; // [local_rows * features]
    double *current_position; // [local_rows * features]
    double *fitness; // [local_rows]

    /* Algorithm execution tmp buffers */
    double *leader; // [features]
    double *roosting_site; // [features]
    double *n_candidate_position; // [features]
    double *direction; // [features]
    int *is_follower; // [local_rows]
    double *prev_location; // [features]
    double *final_location; // [features]

    /* Optional bookkeeping */
    convergence_point_t* convergence_results; // [iterations]
    int num_followers;
    int best_idx;
    double best_fitness;

} prro_state_t;

__attribute__((noreturn)) int err_cleanup_impl(const char *file, int line, const char *func, int mpi_err);

/* Lifecycle */
void mpi_ctx_init(mpi_ctx_t *ctx);
void mpi_ctx_finalize(mpi_ctx_t *ctx, prro_state_t *local);
void mpi_assert_vector(double *vec, int features, const mpi_ctx_t *ctx);

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

