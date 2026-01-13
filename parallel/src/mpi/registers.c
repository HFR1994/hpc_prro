#include "mpi/registers.h"

#include <mpi.h>
#include <stddef.h>   // offsetof

#include "mpi/handlers.h"

/* Define the globals */
MPI_Datatype MPI_LEADER;
MPI_Datatype MPI_METADATA;
MPI_Op       MPI_LEADER_MIN;

/**
 * \brief Custom min function to register to MPI
 * \param in Current element
 * \param inout Next element to compare
 * \param len Length of elements to compare
 * \param dtype Datatype of elements (identifier)
 */
void leader_min(void *in, void *inout, int *len, MPI_Datatype *dtype) {
    leader_t *a = in;
    leader_t *b = inout;

    for (int i = 0; i < *len; i++) {
        if (a[i].fitness < b[i].fitness) {
            b[i].fitness = a[i].fitness;
            b[i].rank    = a[i].rank;
            b[i].index   = a[i].index;
        }
    }
}

void register_leader_struct() {

    const int block_lengths[3] = {1, 1, 1};

    MPI_Aint offsets[3];
    MPI_Datatype types[3] = {
        MPI_DOUBLE,
        MPI_INT,
        MPI_INT,
    };

    offsets[0] = offsetof(leader_t, fitness);
    offsets[1] = offsetof(leader_t, rank);
    offsets[2] = offsetof(leader_t, index);

    MPI_CHECK(MPI_Type_create_struct(3, block_lengths, offsets, types, &MPI_LEADER));
    MPI_CHECK(MPI_Type_commit(&MPI_LEADER));

    MPI_CHECK(MPI_Op_create(leader_min, 1, &MPI_LEADER_MIN));
}