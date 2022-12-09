#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define COUNT 2

int main(int argc, char** argv)
{
    int size, rank;
    MPI_Group full_group;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int group_size = size;
    if (group_size > size)
        group_size = size;
    int* group_array = (int*)malloc(sizeof(int) * group_size);
    for (int i = 0; i < group_size; i++)
        group_array[i] = i;
    MPI_Group group, total_group;
    MPI_Comm_group(MPI_COMM_WORLD, &total_group);
    MPI_Group_incl(total_group, group_size, group_array, &group);
    MPI_Comm bogus;
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
        raise(SIGINT);
    if (rank < group_size)
    {
        MPI_Comm_create_from_group(group, "BANANA", MPI_INFO_NULL, MPI_ERRORS_RETURN, &bogus);
        int banana = 0;
        MPI_Bcast(&banana, 1, MPI_INT, 0, bogus);
    }

    MPI_Finalize();
    return 0;
}