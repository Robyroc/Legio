#include "mpi.h"
#include "mpi-ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char** argv)
{
    int rc;
    int rank, size;
    int send, received = -1;
    double value;
    char errstr[MPI_MAX_ERROR_STRING];

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    send = rank;
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 4) raise(SIGINT);
    int source = (rank-1 < 0 ? size-1 : rank-1);
    int destination = (rank+1 == size ? 0 : rank+1);
    MPI_Request send_req, recv_req;
    MPI_Isend(&send, 1, MPI_INT, destination, 1, MPI_COMM_WORLD, &send_req);
    MPI_Request_free(&send_req);
    MPI_Irecv(&received, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &recv_req);
    MPI_Status send_status, recv_status;

    //MPI_Wait(&send_req, &send_status);
    MPI_Wait(&recv_req, &recv_status);

    MPI_Barrier(MPI_COMM_WORLD);

    if(rank == 3)
        rc = MPI_Send(&send, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
    if(rank == 0)
        rc = MPI_Recv(&received, 1, MPI_INT, 3, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("DATA: (%d/%d)\n", rank, received);

    MPI_Finalize();

    return MPI_SUCCESS;
}
