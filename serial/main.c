#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <utils/logger.h>

#include "include/raven_roost_algorithm.h"
#include "include/utils/dir_file_handler.h"
#include "include/utils/timer.h"
#include "include/utils/pcg_basic.h"

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

    // Set once at program start
    log_set_level(LOG_LEVEL_INFO);
    log_enable_timestamps(1);

    // Seed the random number generator
    pcg32_random_t rng;

    // Seed with current time
    // Best seeds 1768210034 52
    const char *seed_env = getenv("PRRO_SEED");
    uint64_t time_seed = seed_env ? strtoull(seed_env, NULL, 10) : (uint64_t)time(NULL) ^ (uint64_t)clock();
    pcg32_srandom_r(&rng, time_seed, 52u);

    char *placement = getenv("PRRO_PLACEMENT");
    const int np = strtoimax(getenv("NP"), NULL, 10);

    double exec_timings[3];
    exec_timings[0] = get_elapsed_time();

    // Optional output directory: argv[7] or env OUTPUT_DIR or default ./output
    const char *output_dir = NULL;
    if (argc > 9 && argv[8] != NULL && argv[8][0] != '\0') {
        output_dir = argv[8];
    } else {
        const char *env_out = getenv("OUTPUT_DIR");
        if (env_out != NULL && env_out[0] != '\0') output_dir = env_out;
        else output_dir = "./output";
    }

    ensure_dir_exists(output_dir);

    // No defaults here; main requires all args except output_dir
    if (argc < 8) {
        fprintf(stderr, "Usage: %s dataset_path lower_bound upper_bound, iterations, flight_steps, lookout_steps, radius, [output_dir]\n",
                argv[0]);
        return 1;
    }

    const char *dataset_path = argv[1];
    if (dataset_path == NULL || dataset_path[0] == '\0') {
        fprintf(stderr, "Error: dataset_path is empty\n");
        return 1;
    }

    // Parse required positional args
    char *endptr = NULL;
    double lower_bound = strtod(argv[2], &endptr);
    if (endptr == argv[2]) {
        fprintf(stderr, "Error: invalid lower_bound '%s'\n", argv[2]);
        return 1;
    }

    endptr = NULL;
    double upper_bound = strtod(argv[3], &endptr);
    if (endptr == argv[3]) {
        fprintf(stderr, "Error: invalid lower_bound '%s'\n", argv[3]);
        return 1;
    }

    endptr = NULL;
    const long iterations_l = strtol(argv[4], &endptr, 10);
    if (endptr == argv[4] || iterations_l <= 0) {
        fprintf(stderr, "Error: invalid iterations '%s'\n", argv[4]);
        return 1;
    }
    const int iterations = (int) iterations_l;

    endptr = NULL;
    const long flight_steps_l = strtol(argv[5], &endptr, 10);
    if (endptr == argv[5] || flight_steps_l <= 0) {
        fprintf(stderr, "Error: invalid flight_steps '%s'\n", argv[5]);
        return 1;
    }
    const int flight_steps = (int) flight_steps_l;

    endptr = NULL;
    const long lookout_steps_l = strtol(argv[6], &endptr, 10);
    if (endptr == argv[6] || lookout_steps_l <= 0) {
        fprintf(stderr, "Error: invalid lookout_steps '%s'\n", argv[6]);
        return 1;
    }
    const int lookout_steps = (int) lookout_steps_l;

    endptr = NULL;
    const double radius = strtod(argv[7], &endptr);
    if (endptr == argv[7]) {
        fprintf(stderr, "Error: invalid radius '%s'\n", argv[7]);
        return 1;
    }

    if (lower_bound >= upper_bound) {
        fprintf(stderr, "Warning: lower_bound >= upper_bound (%f >= %f). Swapping values.\n", lower_bound, upper_bound);
        const double t = lower_bound;
        lower_bound = upper_bound;
        upper_bound = t;
    }

    int pop_size, features;

    if (parse_dims_from_name(dataset_path, &pop_size, &features) != 0) {
        fprintf(stderr, "Failed to parse dimensions\n");
        exit(EXIT_FAILURE);
    }

    printf("Running serial RRA: dataset=%s, pop_size=%d, features=%d, iter=%d, flight_steps=%d, look_steps=%d, radius=%f, bounds=[%f,%f], output_dir=%s\n",
           dataset_path, pop_size, features, iterations, flight_steps, lookout_steps, radius, lower_bound, upper_bound, output_dir);

    log_info("Random number generator seeded with %lu %lu", time_seed, 52u);

    // // Call the GTO function and time the whole run
    RRA(pop_size, features, iterations, flight_steps, lookout_steps, lower_bound, upper_bound, radius, dataset_path, exec_timings, &rng);

    exec_timings[2] = get_elapsed_time();

    // Log total elapsed

    // write_convergence_csv(convergence_iterations, convergence_values, num_stored, output_dir);

    // Write exec_timings to a log file
    // Self contained
    {
        double const total_time = exec_timings[2] - exec_timings[0];
        double const computation_time = exec_timings[2] - exec_timings[1];

        char filename[1024];
        snprintf(filename, sizeof(filename), "%s/exec_timings_%s_np%d_iter%d_pop%d_feat%d.log", output_dir, placement, np, iterations, pop_size, features);

        FILE *fp = fopen(filename, "w");
        if (fp) {
            fprintf(fp, "total_time: %.10f\n", total_time);
            fprintf(fp, "computation_time: %.10f\n", computation_time);

            fclose(fp);
        } else {
            fprintf(stderr, "Failed to open %s for writing\n", filename);
        }
    }

    printf("\n");

    return 0;
}