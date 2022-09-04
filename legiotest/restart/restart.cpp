#include "mpi.h"
#include "mpi-ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <restart.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    int rank, size, i, first_ranks[2], second_ranks[2], send, received;
    double value;
    char errstr[MPI_MAX_ERROR_STRING];
    MPI_Group world_group, first_group, second_group;
    MPI_Comm first_comm = MPI_COMM_NULL, second_comm = MPI_COMM_NULL;
    MPI_Init(&argc, &argv);


    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (i=0; i < 4; i++) {
        if (i < 2) 
            first_ranks[i] = i;
        else
            second_ranks[i-2] = i;
    }

    printf("Setup done"); fflush(stdout);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    if (rank < 2) {
        MPI_Group_incl( world_group, 2 , first_ranks, &first_group);
        initialize_comm(MPI_COMM_WORLD, first_group, 1, &first_comm);
    }
    else {
        MPI_Group_incl( world_group, 2 , second_ranks, &second_group);
        initialize_comm(MPI_COMM_WORLD, second_group, 2, &second_comm);
    }


    if (!is_respawned()) {
        printf("INSIDE\n");fflush(stdout);
        MPI_Barrier(MPI_COMM_WORLD);

        if (rank == 3)
            raise(SIGINT);
        
        if (rank < 2) 
            MPI_Barrier(first_comm);
        if (rank >=2)
            MPI_Barrier(second_comm);
    }
    sleep(2);
    MPI_Barrier(MPI_COMM_WORLD);
}
