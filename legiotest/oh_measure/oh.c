#include "mpi.h"
#include "mpi-ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char** argv)
{
    int rank, size;
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm bogus;
    double start = MPI_Wtime();
    MPI_Comm_dup(MPI_COMM_WORLD, &bogus);
    double end = MPI_Wtime();

    printf("<%d> Time to dup: %f\n", rank, end-start);
    /*
    start = MPI_Wtime();
    MPI_Comm_free(&bogus);
    end = MPI_Wtime();

    printf("<%d> Time to free: %f\n", rank, end-start);
    */
    
    MPI_Comm bogus2;
    start = MPI_Wtime();
    PMPI_Comm_dup(MPI_COMM_WORLD, &bogus2);
    end = MPI_Wtime();

    printf("<%d> Time to dup original: %f\n", rank, end-start);
    /*
    start = MPI_Wtime();
    PMPI_Comm_free(&bogus2);
    end = MPI_Wtime();

    printf("<%d> Time to free original: %f\n", rank, end-start);
    */
    int value = rank;
    start = MPI_Wtime();
    MPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
    end = MPI_Wtime();

    printf("<%d> Time to bcast: %f\n", rank, end-start);

    value = rank;
    start = MPI_Wtime();
    PMPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
    end = MPI_Wtime();

    printf("<%d> Time to bcast original: %f\n", rank, end-start);

    value = rank;
    int in_value;
    start = MPI_Wtime();
    MPI_Reduce(&value, &in_value, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    end = MPI_Wtime();

    printf("<%d> Time to reduce: %f\n", rank, end-start);

    value = rank;
    start = MPI_Wtime();
    PMPI_Reduce(&value, &in_value, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    end = MPI_Wtime();

    printf("<%d> Time to reduce original: %f\n", rank, end-start);

    start = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    printf("<%d> Time to barrier: %f\n", rank, end-start);

    start = MPI_Wtime();
    PMPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    printf("<%d> Time to barrier original: %f\n", rank, end-start);

    if(rank == 3)
        raise(SIGINT);
    
    start = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    printf("<%d> Time to repair: %f\n", rank, end-start);

    if(rank == 1)
        raise(SIGINT);
    
    start = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    printf("<%d> Time to repair second: %f\n", rank, end-start);

    MPI_Finalize();

    return MPI_SUCCESS;
}