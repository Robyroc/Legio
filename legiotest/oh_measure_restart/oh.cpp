#include "mpi.h"
#include "mpi-ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <restart.h>

#define MULT 100

int print_to_file(double, int, int, FILE*, char*);

int main(int argc, char** argv)
{
    int rank, size;
    
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    FILE* file_p;

    double start;
    double end;

    if (!is_respawned()) {
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if(!is_respawned() && rank == 0)
        raise(SIGINT);
    
    start = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    if(rank == 1)
    {
        file_p = fopen("output.csv", "a");
        fprintf(file_p, "%f\n", end-start);
        fclose(file_p);
    }


    MPI_Finalize();

    return MPI_SUCCESS;
}