#include "mpi.h"
#include "mpi-ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define MULT 1000

int print_to_file(double, int, int, FILE*, char*);

int main(int argc, char** argv)
{
    int rank, size;
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    FILE* file_p;

    if(rank == 1)
    {
        file_p = fopen("output.csv", "w");
    }

    MPI_Comm bogus;
    double start = MPI_Wtime();
    for(int i = 0; i < MULT; i++)
        MPI_Comm_dup(MPI_COMM_WORLD, &bogus);
    double end = MPI_Wtime();
    print_to_file(end-start, rank, size, file_p, "dup");

    /*
    start = MPI_Wtime();
    MPI_Comm_free(&bogus);
    end = MPI_Wtime();

    print_to_file(end-start, rank, size, file_p, "free");
    */
    
    MPI_Comm bogus2;
    start = MPI_Wtime();
    for(int i = 0; i < MULT; i++)
        PMPI_Comm_dup(MPI_COMM_WORLD, &bogus2);
    end = MPI_Wtime();

    print_to_file(end-start, rank, size, file_p, "dup original");
    /*
    start = MPI_Wtime();
    PMPI_Comm_free(&bogus2);
    end = MPI_Wtime();

    print_to_file(end-start, rank, size, file_p, "free original");
    */
    int value = rank;
    start = MPI_Wtime();
    for(int i = 0; i < MULT; i++)
        MPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
    end = MPI_Wtime();

    print_to_file(end-start, rank, size, file_p, "bcast");

    value = rank;
    start = MPI_Wtime();
    for(int i = 0; i < MULT; i++)
        PMPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
    end = MPI_Wtime();

    print_to_file(end-start, rank, size, file_p, "bcast original");

    value = rank;
    int in_value;
    start = MPI_Wtime();
    for(int i = 0; i < MULT; i++)
        MPI_Reduce(&value, &in_value, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    end = MPI_Wtime();

    print_to_file(end-start, rank, size, file_p, "reduce");

    value = rank;
    start = MPI_Wtime();
    for(int i = 0; i < MULT; i++)
        PMPI_Reduce(&value, &in_value, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    end = MPI_Wtime();

    print_to_file(end-start, rank, size, file_p, "reduce original");

    start = MPI_Wtime();
    for(int i = 0; i < MULT; i++)
        MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    print_to_file(end-start, rank, size, file_p, "barrier");

    start = MPI_Wtime();
    for(int i = 0; i < MULT; i++)
        PMPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    print_to_file(end-start, rank, size, file_p, "barrier original");

    if(rank == 0)
        raise(SIGINT);
    
    start = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    print_to_file(end-start, rank, size-1, file_p, "repair");

    if(rank == 2)
        raise(SIGINT);
    
    start = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    print_to_file(end-start, rank, size-2, file_p, "repair again");

    if(rank == 1)
        fclose(file_p);

    MPI_Finalize();

    return MPI_SUCCESS;
}

int print_to_file(double result, int rank, int size, FILE* file_p, char* to_be_printed)
{
    double send_buf = result;
    double recv_buf;
    MPI_Reduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, 1, MPI_COMM_WORLD);
    if(rank == 1)
    {
        recv_buf /= size;
        fprintf(file_p, "%s, %f,\n", to_be_printed, recv_buf);
    }
    MPI_Barrier(MPI_COMM_WORLD);
}