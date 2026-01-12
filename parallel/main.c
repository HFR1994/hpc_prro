#include <mpi.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <utils/logger.h>

#include "include/raven_roost_algorithm.h"
#include "include/utils/dir_file_handler.h"
#include "include/utils/pcg_basic.h"

#include "utils/global.h"
#include "mpi/wrapper.h"

/**
 * \brief Force to print an environment variable
 * \param str Environment Variable name
 * \return value of the environment variable
 */
char *print_env(const char *str) {
    char *env = getenv(str);

    if (env != NULL && env[0] != '\0') {
        log_main("%s: %s", str, env);
    }else {
        ERR_CLEANUP()
    }

    return env;
}

int do_work(int world_rank, int world_size){

    log_main("");

    const int pop_size = 129;

    const int rows_per_rank = pop_size / world_size;
    const int remainder     = pop_size % world_size;

    const int local_rows = rows_per_rank + (world_rank < remainder);
    const int start_row = world_rank * rows_per_rank + (world_rank < remainder ? world_rank : remainder);

    log_info("World Rank: %d, World Size: %d, Rows per Rank: %d, Remainder: %d, Local Rows: %d, Start Row: %d", world_rank, world_size, rows_per_rank, remainder, local_rows, start_row);

    MPI_Finalize();
    return EXIT_SUCCESS;
}

/*
 * Main implementation
 * Receives
 *  1. CSV file with NxM rows and columns, each row represents a Raven and each column a dimension
 *  2. Lower bound namespace equaled to the Raven features
 *  3. Upper bound namespace equaled to the Raven features
 *  4. Number of iterations per Raven
 *  5. Number of steps to reach destination
 *  6. Number of search steps per Raven
 *  7. Looking Radius
 *  8. Output Directory
 */
int main(int argc, char **argv) {

    MPI_CHECK(MPI_Init(&argc, &argv));
    mpi_ctx_t ctx;
    mpi_ctx_init(&ctx);

    MPI_CHECK(MPI_Barrier(ctx.comm));

    double exec_timings[3];
    exec_timings[0] = MPI_Wtime();

    log_enable_timestamps(1);

    char *placement = NULL;
    
    // Set once at program start, identify which is the main process
    // Set once at program start
    if (ctx.rank != 0) {
        log_set_level(LOG_LEVEL_ERR);
    }else {
        log_set_level(LOG_LEVEL_INFO);
    }

    log_world_rank(ctx.rank);

    log_main("MPI world size: %d", ctx.size);

    print_env("PRRO_TRIAL");
    print_env("PRRO_EXECUTION");
    placement = print_env("PRRO_PLACEMENT");
    
    // Seed the random number generator
    pcg32_random_t rng;

    // Seed with current time
    // Best seeds 1768210034 52
    const char *seed_env = getenv("PRRO_SEED");
    uint64_t time_seed = seed_env ? strtoull(seed_env, NULL, 10) : (uint64_t)time(NULL) ^ (uint64_t)clock();
    pcg32_srandom_r(&rng, time_seed + ctx.rank, 52u);

    // return do_work(world_rank, world_size);
    
    // Optional output directory: argv[7] or env OUTPUT_DIR or default ./output
    const char *output_dir = NULL;
    if (argc > 8 && argv[8] != NULL && argv[8][0] != '\0') {
        output_dir = argv[8];
    } else {
        const char *env_out = getenv("OUTPUT_DIR");
        if (env_out != NULL && env_out[0] != '\0') output_dir = env_out;
        else output_dir = "./output";
    }

    ensure_dir_exists(output_dir);

    // No defaults here; main requires all args except output_dir
    if (argc < 8) {
        log_err("Usage: %s dataset_path lower_bound upper_bound, iterations, flight_steps, lookout_steps, radius, [output_dir]\n",
                argv[0]);
        ERR_CLEANUP();        
    }

    const char *dataset_path = argv[1];
    if (dataset_path == NULL || dataset_path[0] == '\0') {
        log_err("Error: dataset_path is empty\n");
        ERR_CLEANUP();
    }

    // Parse required positional args
    char *endptr = NULL;
    double lower_bound = strtod(argv[2], &endptr);
    if (endptr == argv[2]) {
        log_err("Error: invalid lower_bound '%s'\n", argv[2]);
        ERR_CLEANUP();
    }

    endptr = NULL;
    double upper_bound = strtod(argv[3], &endptr);
    if (endptr == argv[3]) {
        log_err("Error: invalid lower_bound '%s'\n", argv[3]);
        ERR_CLEANUP();
    }

    endptr = NULL;
    const long iterations_l = strtol(argv[4], &endptr, 10);
    if (endptr == argv[4] || iterations_l <= 0) {
        log_err("Error: invalid iterations '%s'\n", argv[4]);
        ERR_CLEANUP();
    }
    const int iterations = (int) iterations_l;

    endptr = NULL;
    const long flight_steps_l = strtol(argv[5], &endptr, 10);
    if (endptr == argv[5] || flight_steps_l <= 0) {
        log_err("Error: invalid flight_steps '%s'\n", argv[5]);
        ERR_CLEANUP();
    }
    const int flight_steps = (int) flight_steps_l;

    endptr = NULL;
    const long lookout_steps_l = strtol(argv[6], &endptr, 10);
    if (endptr == argv[6] || lookout_steps_l <= 0) {
        log_err("Error: invalid lookout_steps '%s'\n", argv[6]);
        ERR_CLEANUP();
    }
    const int lookout_steps = (int) lookout_steps_l;

    endptr = NULL;
    const double radius = strtod(argv[7], &endptr);
    if (endptr == argv[7]) {
        log_err("Error: invalid radius '%s'\n", argv[7]);
        ERR_CLEANUP();
    }

    if (lower_bound >= upper_bound) {
        log_warning("Warning: lower_bound >= upper_bound (%f >= %f). Swapping values.\n", lower_bound, upper_bound);
        const double t = lower_bound;
        lower_bound = upper_bound;
        upper_bound = t;
    }

    int pop_size, features;

    if (parse_dims_from_name(dataset_path, &pop_size, &features) != 0) {
        log_err("Failed to parse dimensions\n");
        ERR_CLEANUP();
    }

    const char *env_out = getenv("MEASURE_SPEEDOUT");
    const bool is_measure_speedup = str_to_bool(env_out, false);

    if (is_measure_speedup) {
        log_main("Speedup measurement enabled\n");
    }

    log_main("Running parallel RRA: dataset=%s, pop_size=%d, features=%d, iter=%d, flight_steps=%d, look_steps=%d, radius=%f, bounds=[%f,%f], output_dir=%s\n",
           dataset_path, pop_size, features, iterations, flight_steps, lookout_steps, radius, lower_bound, upper_bound, output_dir);

    log_main("Random number generator seeded with %lu %lu", time_seed, 52u);

    // // Call the GTO function and time the whole run
    RRA(pop_size, features, iterations, flight_steps, lookout_steps, lower_bound, upper_bound, radius, dataset_path, exec_timings, is_measure_speedup, &rng, &ctx);

    MPI_CHECK(MPI_Barrier(ctx.comm));
    exec_timings[2] = MPI_Wtime();

    // Log total elapsed

    // write_convergence_csv(convergence_iterations, convergence_values, num_stored, output_dir);

    // Write exec_timings to a log file
    // Self contained
    {
        double const local_total = exec_timings[2] - exec_timings[0];
        double const local_compute = exec_timings[2] - exec_timings[1];

        double global_total = 0.0;
        double global_compute = 0.0;

        MPI_CHECK(MPI_Reduce(&local_total, &global_total, 1,
                   MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD));

        MPI_CHECK(MPI_Reduce(&local_compute, &global_compute, 1,
                   MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD));

        char filename[1024];
        if (ctx.rank == 0) {
            snprintf(filename, sizeof(filename), "%s/exec_timings_%s_np%d_iter%d_pop%d_feat%d.log", output_dir, placement, ctx.size, iterations, pop_size, features);

            FILE *fp = fopen(filename, "w");
            if (fp) {
                // To aggregate on the logs
                log_main("total_time: %.10f", global_total);
                log_main("computation_time: %.10f", global_compute);

                fprintf(fp, "total_time: %.10f\n", global_total);
                fprintf(fp, "computation_time: %.10f\n", global_compute);

                fclose(fp);
            }else {
                log_err("Failed to open %s for writing\n", filename);
            }
        }
    }

    mpi_ctx_finalize(&ctx);
    MPI_Finalize();
    return EXIT_SUCCESS;
}