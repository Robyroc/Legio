#include "mpi.h"
#include "mpi-ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <restart.h>
#include <unistd.h>

// Run with `-n 3 --to-respawn 0`
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

    printf("Setup done for rank: %d\n", rank); fflush(stdout);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_size(world_group, &world_group_size);
    initialize_comm(2, first_ranks, &first_comm);

    if (!is_respawned() && rank <= 1) {
        if (rank == 0) {
            raise(SIGINT);
        }
        printf("[1-%d] ORIGINAL BEFORE BARRIER\n", rank); fflush(stdout);
        MPI_Barrier(first_comm);
        printf("[1-%d] ORIGINAL AFTER BARRIER\n", rank); fflush(stdout);
    }
    else if (is_respawned() && rank == 0) {
        printf("[0-%d] RESPAWNED BEFORE BARRIER\n", rank); fflush(stdout);
        MPI_Barrier(first_comm);
        printf("[0-%d] RESPAWNED AFTER BARRIER\n", rank); fflush(stdout);
    }
    else {
        printf("[2-%d] SLEEEEEPING\n", rank); fflush(stdout);
        sleep(5);

    }
    printf("NEXT SHOULD SUCCEED FOR ALL THREE RANKS\n");
    MPI_Barrier(MPI_COMM_WORLD);
}
