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

int cleanup() {
    MPI_Finalize();
    return EXIT_SUCCESS;
}

int err_cleanup(void) {
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    return EXIT_FAILURE; // unreachable, but explicit
}


void print_env(const char * str) {
    const char *env = getenv(str);

    if (env != NULL && env[0] != '\0') {
        log_info("%s: %s", str, env);
    }
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

    int world_size, world_rank;

    int ierr = MPI_Init(&argc, &argv);

    if (ierr != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Init failed\n");
        return 1;
    }

    ierr = MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (ierr != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Comm_size failed\n");
        return err_cleanup();
    }

    ierr = MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (ierr != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Comm_size failed\n");
        return err_cleanup();
    }

    MPI_Barrier(MPI_COMM_WORLD);

    double exec_timings[3];
    exec_timings[0] = MPI_Wtime();

    log_enable_timestamps(1);

    // Set once at program start
    if (world_rank != 0) {
        log_set_level(LOG_LEVEL_ERR);
    }else {
        log_set_level(LOG_LEVEL_INFO);
        log_info("MPI world size: %d", world_size);

        print_env("PRRO_TRIAL");
        print_env("PRRO_EXECUTION");
        print_env("PRRO_PLACEMENT");
    }

    // Seed the random number generator
    pcg32_random_t rng;

    // Seed with current time
    // Best seeds 1768210034 52
    const char *seed_env = getenv("PRRO_SEED");
    uint64_t time_seed = seed_env ? strtoull(seed_env, NULL, 10) : (uint64_t)time(NULL) ^ (uint64_t)clock();
    pcg32_srandom_r(&rng, time_seed + world_rank, 52u);

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
        return err_cleanup();        
    }

    const char *dataset_path = argv[1];
    if (dataset_path == NULL || dataset_path[0] == '\0') {
        log_err("Error: dataset_path is empty\n");
        return err_cleanup();
    }

    // Parse required positional args
    char *endptr = NULL;
    double lower_bound = strtod(argv[2], &endptr);
    if (endptr == argv[2]) {
        log_err("Error: invalid lower_bound '%s'\n", argv[2]);
        return err_cleanup();
    }

    endptr = NULL;
    double upper_bound = strtod(argv[3], &endptr);
    if (endptr == argv[3]) {
        log_err("Error: invalid lower_bound '%s'\n", argv[3]);
        return err_cleanup();
    }

    endptr = NULL;
    const long iterations_l = strtol(argv[4], &endptr, 10);
    if (endptr == argv[4] || iterations_l <= 0) {
        log_err("Error: invalid iterations '%s'\n", argv[4]);
        return err_cleanup();
    }
    const int iterations = (int) iterations_l;

    endptr = NULL;
    const long flight_steps_l = strtol(argv[5], &endptr, 10);
    if (endptr == argv[5] || flight_steps_l <= 0) {
        log_err("Error: invalid flight_steps '%s'\n", argv[5]);
        return err_cleanup();
    }
    const int flight_steps = (int) flight_steps_l;

    endptr = NULL;
    const long lookout_steps_l = strtol(argv[6], &endptr, 10);
    if (endptr == argv[6] || lookout_steps_l <= 0) {
        log_err("Error: invalid lookout_steps '%s'\n", argv[6]);
        return err_cleanup();
    }
    const int lookout_steps = (int) lookout_steps_l;

    endptr = NULL;
    const double radius = strtod(argv[7], &endptr);
    if (endptr == argv[7]) {
        log_err("Error: invalid radius '%s'\n", argv[7]);
        return err_cleanup();
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
        return err_cleanup();
    }

    const char *env_out = getenv("MEASURE_SPEEDOUT");
    const bool is_measure_speedup = str_to_bool(env_out, false);

    if (is_measure_speedup) {
        log_info("Speedup measurement enabled\n");
    }

    log_info("Running parallel RRA: dataset=%s, pop_size=%d, features=%d, iter=%d, flight_steps=%d, look_steps=%d, radius=%f, bounds=[%f,%f], output_dir=%s\n",
           dataset_path, pop_size, features, iterations, flight_steps, lookout_steps, radius, lower_bound, upper_bound, output_dir);

    log_info("Random number generator seeded with %lu %lu", time_seed, 52u);

    // // Call the GTO function and time the whole run
    RRA(pop_size, features, iterations, flight_steps, lookout_steps, lower_bound, upper_bound, radius, dataset_path, exec_timings, is_measure_speedup, &rng);

    MPI_Barrier(MPI_COMM_WORLD);
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

        MPI_Reduce(&local_total, &global_total, 1,
                   MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        MPI_Reduce(&local_compute, &global_compute, 1,
                   MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        char filename[1024];
        snprintf(filename, sizeof(filename), "%s/exec_timings_np%d_iter%d_pop%d_feat%d.log", output_dir, world_size, iterations, pop_size, features);
        if (world_rank == 0) {
            FILE *fp = fopen(filename, "w");
            if (fp) {
                // To aggregate on the logs
                log_info("total_time: %.10f\n", global_total);
                log_info("computation_time: %.10f\n", global_compute);

                fprintf(fp, "total_time: %.10f\n", global_total);
                fprintf(fp, "computation_time: %.10f\n", global_compute);

                fclose(fp);
            }else {
                fprintf(stderr, "Failed to open %s for writing\n", filename);
            }
        }
    }

    printf("\n");

    return cleanup();
}