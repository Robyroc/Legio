#include "mpi.h"
#include "mpi-ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <restart.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    int rank, size, i, first_ranks[2], second_ranks[2], send, received, world_group_size;
    double value;
    char errstr[MPI_MAX_ERROR_STRING];
    MPI_Group world_group, first_group, second_group;
    MPI_Comm first_comm = MPI_COMM_NULL, second_comm = MPI_COMM_NULL;
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("HERE"); fflush(stdout);

    for (i=0; i < 4; i++) {
        if (i < 2) 
            first_ranks[i] = i;
        else
            second_ranks[i-3] = i;
    }

    first_ranks[0] = 0;
    first_ranks[1] = 1;
    printf("Setup done"); fflush(stdout);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_size(world_group, &world_group_size);
    printf("%d\n", rank);
    initialize_comm(2, first_ranks, &first_comm);
    MPI_Barrier(first_comm);

    if (rank == 0) {
        raise(SIGINT);
    }

    if (!is_respawned()) {
        printf("INSIDE\n");fflush(stdout);
        MPI_Barrier(first_comm);
    }
    else {
        printf("RESPAWNED!");
    }
    MPI_Barrier(MPI_COMM_WORLD);
}
