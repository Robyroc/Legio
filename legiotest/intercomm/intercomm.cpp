#include "mpi.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

int main(int argc, char* argv[] )
{
    int world_rank, world_size;
    MPI_Comm split_world, group_comm = MPI_COMM_NULL;
    MPI_Group world_group, shrink_group;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );
    MPI_Comm_size( MPI_COMM_WORLD, &world_size );

    MPI_Comm_split(MPI_COMM_WORLD, world_rank%2, 0, &split_world);
    MPI_Barrier(split_world);
    if(world_rank == 2) raise(SIGINT);
    MPI_Comm icomm;
    if(world_rank%2)
        MPI_Intercomm_create(split_world, 0, MPI_COMM_WORLD, 0, 0, &icomm);
    else
        MPI_Intercomm_create(split_world, 0, MPI_COMM_WORLD, 1, 0, &icomm);
    MPI_Comm merged;
    MPI_Intercomm_merge(icomm, world_rank%2, &merged);
    int new_rank;
    MPI_Comm_rank(merged, &new_rank);
    printf("Old rank was %d, new rank is %d\n", world_rank, new_rank);
    MPI_Comm_free(&merged);
    MPI_Comm_free(&split_world);
    MPI_Finalize();
    return 0;
}
