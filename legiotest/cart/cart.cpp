#include <signal.h>
#include <cstdio>
#include "mpi.h"

#include "mpi-ext.h"

int main(int argc, char** argv)
{
    int rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm topo;
    int size;
    int dims[2];
    dims[0] = 0;
    dims[1] = 0;
    int periods[2];
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Dims_create(size, 2, dims);
    printf("%d %d\n", dims[0], dims[1]);
    periods[0] = 1;
    periods[1] = 1;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &topo);
    int source, dest, value = rank;
    MPI_Cart_shift(topo, 0, 1, &source, &dest);
    MPI_Sendrecv_replace(&value, 1, MPI_INT, dest, 0, source, 0, topo, MPI_STATUS_IGNORE);
    printf("A %d: %d\n", rank, value);
    value = rank;
    if (rank == 3)
        raise(SIGINT);
    // MPI_Barrier(topo);
    if (rank == 2)
    {
        int rc = MPI_Sendrecv_replace(&value, 1, MPI_INT, 3, 0, 3, 1, topo, MPI_STATUS_IGNORE);
        int len;
        char errstr[MPI_MAX_ERROR_STRING];
        MPI_Error_string(rc, errstr, &len);
        printf("%s\n", errstr);
    }
    MPI_Cart_shift(topo, 0, 1, &source, &dest);
    MPI_Sendrecv_replace(&value, 1, MPI_INT, dest, 0, source, 0, topo, MPI_STATUS_IGNORE);
    printf("B %d: %d\n", rank, value);
    MPI_Finalize();
    // MPI_Barrier(temp);
    return 0;
}
