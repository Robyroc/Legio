#include "mpi.h"
#ifdef MPICH
#include "mpi_proto.h"
#else
#include "mpi-ext.h"
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    int rank, size;
    int send, received;
    double value;
    char errstr[MPI_MAX_ERROR_STRING];

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    send = rank;
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 4)
        raise(SIGINT);

    MPI_Barrier(MPI_COMM_WORLD);

    value = rank / (double)size;

    if (rank == (size / 4))
        raise(SIGINT);
    MPI_Bcast(&value, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (value != 0.0)
    {
        printf("Rank %d / %d: value from %d is wrong: %g\n",  // try what happens without ft
               rank, size, 0, value);                         // with test 5
    }
    if (rank == 0)
        MPI_Send(&send, 1, MPI_INT, 4, 1, MPI_COMM_WORLD);
    if (rank == 4)
        MPI_Recv(&received, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, NULL);
    MPI_Finalize();

    return MPI_SUCCESS;
}
