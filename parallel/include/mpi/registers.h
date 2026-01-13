#ifndef RRA_PARALLEL_REGISTERS_H
#define RRA_PARALLEL_REGISTERS_H
#include <mpi.h>

/* Exposed MPI objects */
extern MPI_Datatype MPI_LEADER;
extern MPI_Op       MPI_LEADER_MIN;

typedef struct {
    double fitness;  // comparison key
    int rank;     // owning rank
    int index;    // local index on that rank
    double* location; // Leader location
} leader_t;

void register_leader_struct();

#endif //RRA_PARALLEL_REGISTERS_H