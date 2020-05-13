#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define COUNT 2

int main(int argc, char **argv) {
    int size, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int* globaldata = malloc(sizeof(int) * size * COUNT);/*wants to declare array this way*/
    int localdata[COUNT];/*without using pointers*/

    int i;
    if (rank == 0) {

        for (i=0; i<size * COUNT; i++)
            globaldata[i] = i;

        printf("1. Processor %d has data: ", rank);
        for (i=0; i<size*COUNT; i++)
            printf("%d ", globaldata[i]);
        printf("\n");
    }

    if(rank == 4)
        raise(SIGINT);

    MPI_Scatter(globaldata, COUNT, MPI_INT, &localdata, COUNT, MPI_INT, 0, MPI_COMM_WORLD);

    for(int i = 0; i < COUNT; i++)
        localdata[i]*=2;

    MPI_Gather(&localdata, COUNT, MPI_INT, globaldata, COUNT, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("4. Processor %d has data: ", rank);
        for (i=0; i<size * COUNT; i++)
            printf("%d ", globaldata[i]);
        printf("\n");
    }


    MPI_Finalize();
    return 0;
}