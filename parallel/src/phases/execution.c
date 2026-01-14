#include "../../include/phases/execution.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "mpi/registers.h"

#include "utils/global.h"
#include "utils/logger.h"
#include "utils/number_generators.h"

/**
 * \brief Send the raven back to the meeting point to start a new journey
 * \param local Local state for this rank
 * \param global Global configuration
 */
void gather_to_roosting(prro_state_t * local, const prra_cfg_t global) {
    // Set each position to the roosting site
    for (int i = 0; i < local->local_rows; i++) {
        memcpy(&local->current_position[i * global.features], local->roosting_site, global.features * sizeof(double));
    }
}

/**
 * \brief Define which are the followers, excluding the leader
 * \param local Local state for this rank
 * \param global Global configuration
 * \param current_leader Struct with the global leader
 * \param rng The random number generator
 */
void define_followers(prro_state_t * local, prra_cfg_t global, const leader_t current_leader, pcg32_random_t *rng, const mpi_ctx_t * ctx) {

    int *counts = NULL;
    int *displs = NULL;
    int* followers = NULL;

    if (global.pop_size > 1) {
        if (ctx->rank == 0) {
            // Initialize the array to 0's
            followers = malloc(global.pop_size * sizeof(int));
            int *available = malloc(global.pop_size * sizeof(int));
            memset(followers, 0, global.pop_size * sizeof(int));

            if (!available || !followers) {
                free(available);
                free(followers);
                log_err("Failed to allocate memory for available indices");
                ERR_CLEANUP();
            }

            int count = 0;

            // Create a list of available indices (excluding current_leader) if it's part of this rank
            for (int i = 0; i < global.pop_size; i++) {
                if (i != current_leader.index) {
                    available[count++] = i;
                }
            }

            // Shuffle available indices using Fisher-Yates
            for (int i = count - 1; i > 0; i--) {
                const int j = (int) (unif_0_1(rng) * (i + 1));
                const int temp = available[i];
                available[i] = available[j];
                available[j] = temp;
            }

            const int num_followers = ceil(0.2 * global.pop_size - 1);

            // Set first num_followers indices to 1
            for (int i = 0; i < num_followers && i < count; i++) {
                followers[available[i]] = 1;
            }

            counts = malloc(ctx->size * sizeof(int));
            displs = malloc(ctx->size * sizeof(int));

            for (int i = 0; i < ctx->size; i++) {
                const metadata_state_t metadata = get_bounds(global, ctx->size, i);
                counts[i] = metadata.local_rows;
                displs[i] = metadata.start_row;
            }

            free(available);
        }

        MPI_CHECK(MPI_Scatterv(
            followers,   // send buffer (root only)
            counts, // send counts per rank
            displs, // displacements
            MPI_INT, // datatype
            local->is_follower, // receive buffer (local)
            local->local_rows,  // receive count
            MPI_INT,
            0, // root
            ctx->comm
        ));

        int total_followers = 0;
        for (int i = 0; i < local->local_rows; i++) {
            if (local->is_follower[i] == 1) {
                total_followers += 1;
            }
        }

        local->num_followers = total_followers;

        free(counts);
        free(displs);
        free(followers);
    }
}

/**
 * \brief Calculate a new lookout position around the current position based on hypersphere (N dimensions) radii
 * \param local Local state for this rank
 * \param global Global configuration
 * \param current_position The N-Dimension current position array for each raven
 * \param rPcpt The radii of look
 * \param rng The random state generator
 */
void set_lookout(prro_state_t * local, const prra_cfg_t global, const double* current_position, const double rPcpt, pcg32_random_t *rng) {

    // Vector needs to be biased to the center (Normal distribution) to
    // have a uniform direction on the hypersphere
    const double distance = vector_to_distance(local->n_candidate_position, global.features, rng, true);
    gen_unit_vector(local->n_candidate_position, distance, global.features);

    const double U = unif_0_1(rng);
    const double r = rPcpt * pow(U, 1.0 / global.features);

    // "Makes random perceptions within this hypersphere" (Use Euclidean distance)
    for (int j = 0; j < global.features; j++) {
        local->n_candidate_position[j] = current_position[j] + r * local->n_candidate_position[j];
    }
}
