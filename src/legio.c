#include "legio.h"
#include "mpi.h"
#include "mpi-ext.h"


void fault_number(MPI_Comm comm, int* size)
{
    MPI_Group failed;
    MPIX_Comm_failure_get_acked(comm, &failed);
    MPI_Group_size(failed, size);
}

void who_failed(MPI_Comm comm, int* size, int* ranks)
{
    MPI_Group failed, comm_group;
    MPIX_Comm_failure_get_acked(comm, &failed);
    MPI_Group_size(failed, size);
    MPI_Comm_group(comm, &comm_group);
    int i;
    for(i = 0; i < *size && i < LEGIO_MAX_FAILS; i++)
        MPI_Group_translate_ranks(failed, 1, &i, comm_group, &(ranks[i]));
}