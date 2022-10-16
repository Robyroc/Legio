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


    printf("Setup done"); fflush(stdout);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_size(world_group, &world_group_size);
    printf("%d\n", world_group_size);
    if (rank < 2) {
        initialize_comm(2, first_ranks, &first_comm);
    }

    if (!is_respawned()) {
        printf("INSIDE\n");fflush(stdout);
        MPI_Barrier(MPI_COMM_WORLD);
    }
    else {
        printf("RESPAWNED!");
    }
    MPI_Barrier(MPI_COMM_WORLD);
}
