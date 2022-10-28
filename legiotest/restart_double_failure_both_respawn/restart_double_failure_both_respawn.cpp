#include "mpi.h"
#include "mpi-ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <restart.h>
#include <unistd.h>

// Run with `-n 3 --to-respawn 0,1` or `-n 3 --to-respawn 0,1`
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

    printf("\n\nSetup done for rank: %d\n", rank); fflush(stdout);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_size(world_group, &world_group_size);
    initialize_comm(3, first_ranks, &first_comm);
    
    if (is_respawned() && rank == 0) {
        int rankk;
        MPI_Comm_rank(MPI_COMM_WORLD, &rankk);
        MPI_Barrier(first_comm);
        printf("\n[rank 0] Respawned after first barrier\n"); fflush(stdout);
        MPI_Barrier(first_comm);
        printf("\n[rank 0] Respawned after second barrier\n"); fflush(stdout);
    }
    else if (is_respawned() && rank == 1) {
        int rankk;
        MPI_Comm_rank(MPI_COMM_WORLD, &rankk);
        MPI_Barrier(first_comm);
        printf("\n[rank 1] Respawned after second barrier\n"); fflush(stdout);
    }
    else {
        if (rank == 0) {
            raise(SIGINT);
        }
        MPI_Barrier(first_comm);
        printf("\n[rank 1-2] Original after first barrier\n"); fflush(stdout);
        if (rank == 1) {
            raise(SIGINT);
        }
        MPI_Barrier(first_comm);
        printf("\n[rank 2] Original after second barrier\n"); fflush(stdout);
        }
    MPI_Barrier(MPI_COMM_WORLD);
    int cur_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &cur_rank);
    printf("Last barrier done, rank: %d", cur_rank); fflush(stdout);
    }
