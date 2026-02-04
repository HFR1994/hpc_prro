#ifndef RRA_PARALLEL_OBJECTIVE_FUNCTION_H
#define RRA_PARALLEL_OBJECTIVE_FUNCTION_H

#include "mpi/handlers.h"

/**
 * \brief Use the Griewank function as an objective function
 * \param X The raven's current position in N dimensions
 * \param global Global configuration
 * \return The objective function value
 */
double objective_function(const double *X, const prra_cfg_t global);

/**
 * \brief Set the leader to the raven with the lowest fitness value
 * \param local Local state for this rank
 * \param global Global configuration
 * \return The index of the leader in the local population
 */
int set_leader(prro_state_t * local, const prra_cfg_t global);

#endif //RRA_PARALLEL_OBJECTIVE_FUNCTION_H