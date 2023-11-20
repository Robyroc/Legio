#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

/* A two-dimensional torus of 12 processes in a 4x3 grid */
int main(int argc, char* argv[])
{
    int rank, size;
    MPI_Comm comm;
    int dim[2], period[2], reorder;
    int coord[2], id;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    dim[0] = 2;
    dim[1] = 3;
    period[0] = 1;
    period[1] = 0;
    reorder = 0;
    int rc = MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, reorder, &comm);
    rc = MPI_Cart_coords(comm, rank, 2, coord);
    printf("Rank %d coordinates are %d %d\n", rank, coord[0], coord[1]);
    fflush(stdout);
    if (rank == 0)
    {
        coord[0] = 1;
        coord[1] = 2;
        MPI_Cart_rank(comm, coord, &id);
        printf("The processor at position (%d, %d) has rank %d\n", coord[0], coord[1], id);
        fflush(stdout);
    }

    MPI_Finalize();
    return 0;
}