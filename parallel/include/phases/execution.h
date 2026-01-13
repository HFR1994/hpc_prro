#ifndef RRA_PARALLEL_EXECUTION_H
#define RRA_PARALLEL_EXECUTION_H

#include "mpi/wrapper.h"
#include "utils/pcg_basic.h"

/**
 * \brief Define which are the followers, excluding the leader
 * \param local Local state for this rank
 * \param current_leader Index of the current leader
 * \param rng The random number generator
 */
void define_followers(prro_state_t * local, int current_leader, pcg32_random_t *rng);

/**
 * \brief Calculate a new lookout position around the current position based on hypersphere (N dimensions) radii
 * \param local Local state for this rank
 * \param global Global configuration
 * \param current_position The N-Dimension current position array for each raven
 * \param rPcpt The radii of look
 * \param rng The random state generator
 */
void set_lookout(prro_state_t * local, const prra_cfg_t global, const double* current_position, double rPcpt, pcg32_random_t *rng);

/**
 * \brief Send the raven back to the meeting point to start a new journey
 * \param local Local state for this rank
 * \param global Global configuration
 */
void gather_to_roosting(prro_state_t * local, const prra_cfg_t global);

#endif //RRA_PARALLEL_EXECUTION_H