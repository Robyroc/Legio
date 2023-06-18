#include "mpi.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <array>
extern "C"{
#include "restart.h"
}

int main(int argc, char* argv[] )
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if(argc < 2)
    {
        if(rank == 0)
            printf("Need number of communicators to create");
        MPI_Finalize();
        return 0;
    }
    int jumps;
    if(argc > 2)
    {
        jumps = atoi(argv[2]);
        if(jumps == 0)
            jumps = 1;
    }
    else
        jumps = 1;
    int ranks[size];
    for(int i = 0; i < size; i++)
        ranks[i] = i;
    int comm_number = atoi(argv[1]);
    MPI_Comm comms[comm_number];
    for(int i = 0; i < comm_number; i++)
        initialize_comm(size, ranks, &(comms[i]));
    MPI_Barrier(comms[0]);
    double times[comm_number];
    if(rank == 1)
        raise(SIGINT);
    for(int i = 0; i < comm_number; i += jumps)
    {
        double start = MPI_Wtime();
        MPI_Barrier(comms[i]);
        double end = MPI_Wtime();
        times[i] = end-start;
    }
    double results[comm_number];
    MPI_Reduce(times, results, comm_number, MPI_DOUBLE, MPI_SUM, 0, comms[0]);
    if(rank == 0)
    {
        for(int i = 0; i < comm_number; i+= jumps)
        {
            results[i] /= (size - 1);
            printf("%f\n", results[i]);        
        }
    }
    MPI_Finalize();
    return 0;
}
