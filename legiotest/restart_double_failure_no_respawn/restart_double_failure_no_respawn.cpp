#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mpi.h"
extern "C" {
#include "restart.h"
}

// Run with `-n 3`
int main(int argc, char** argv)
{
    int rank, size, i, first_ranks[3], second_ranks[2], send, received, world_group_size;
    double value;
    char errstr[MPI_MAX_ERROR_STRING];
    MPI_Group world_group, first_group, second_group;
    MPI_Comm first_comm = MPI_COMM_NULL, second_comm = MPI_COMM_NULL;
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    first_ranks[0] = 0;
    first_ranks[1] = 1;
    first_ranks[2] = 2;

    printf("Setup done for rank: %d\n", rank);
    fflush(stdout);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_size(world_group, &world_group_size);
    initialize_comm(3, first_ranks, &first_comm);

    MPI_Barrier(first_comm);

    if (rank == 0)
    {
        raise(SIGINT);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 1)
    {
        raise(SIGINT);
    }

    MPI_Barrier(first_comm);
}
