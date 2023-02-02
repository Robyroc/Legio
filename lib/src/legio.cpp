extern "C" {
#include "legio.h"
}
#include "intercomm_utils.hpp"
#include "mpi.h"
#include "multicomm.hpp"

#include "mpi-ext.h"

void fault_number(MPI_Comm comm, int* size)
{
    MPI_Group failed;
    MPIX_Comm_failure_get_acked(comm, &failed);
    PMPI_Group_size(failed, size);
}

void who_failed(MPI_Comm comm, int* size, int* ranks)
{
    MPI_Group failed, comm_group;
    MPIX_Comm_failure_get_acked(comm, &failed);
    PMPI_Group_size(failed, size);
    PMPI_Comm_group(comm, &comm_group);
    int i;
    for (i = 0; i < *size && i < LEGIO_MAX_FAILS; i++)
        PMPI_Group_translate_ranks(failed, 1, &i, comm_group, &(ranks[i]));
}

int MPIX_Comm_agree_group(MPI_Comm comm, MPI_Group group, int* flag)
{
    *flag = non_collective_agree(group, comm, *flag);
    return MPI_SUCCESS;
}

int MPIX_Horizon_from_group(MPI_Group group)
{
    MPI_Comm horizon;
    int rc = PMPI_Comm_create_from_group(group, "Legio_horizon_construction", MPI_INFO_NULL,
                                         MPI_ERRORS_RETURN, &horizon);
    Multicomm::get_instance().add_horizon_comm(horizon);
    return rc;
}