#include "mpi.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <array>
extern "C"{
#include "restart.h"
}

#define COMM_NUMBER 5

int main(int argc, char* argv[] )
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int ranks[size];
    for(int i = 0; i < size; i++)
        ranks[i] = i;
    int comm_number = COMM_NUMBER;
    MPI_Comm comms[comm_number];
    for(int i = 0; i < comm_number; i++)
        initialize_comm(size, ranks, &(comms[i]));

    if(rank == 1 && !is_respawned())
        raise(SIGINT);
    double start = MPI_Wtime();
    MPI_Barrier(comms[0]);
    double end = MPI_Wtime();
    double time = end-start;

    double result;
    MPI_Reduce(&time, &result, 1, MPI_DOUBLE, MPI_SUM, 0, comms[0]);
    if(rank == 0)
        printf("%f\n", result/size);        
    MPI_Finalize();
    return 0;
}
