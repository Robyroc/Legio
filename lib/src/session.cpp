#include <signal.h>
#include <future>
#include <shared_mutex>
#include <thread>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "intercomm_utils.hpp"
#include "log.hpp"
#include "mpi.h"
#include "multicomm.hpp"
#include "restart_routines.hpp"

extern std::shared_timed_mutex failure_mtx;
using namespace legio;

void check_group(MPI_Comm cur_comm, MPI_Group group, MPI_Group* clean)
{
    if constexpr (BuildOptions::cube_algorithm)
        *clean = deeper_check_cube(group, cur_comm);
    else
        *clean = deeper_check_tree(group, cur_comm);
}

int MPI_Session_init(MPI_Info info, MPI_Errhandler errhandler, MPI_Session* session)
{
    if constexpr (BuildOptions::with_restart)
        assert(false && "Session model incompatible with restart functionalities");
    int flag;
    MPI_Initialized(&flag);
    if (!Multicomm::get_instance().is_initialized())
    {
        MPI_Session temp;
        int flag2, size;
        MPI_Group group;
        PMPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &temp);
        PMPI_Group_from_session_pset(temp, "mpi://WORLD", &group);
        PMPI_Group_size(group, &size);
        PMPI_Group_free(&group);
        Multicomm::get_instance().initialize(size);
        // PMPI_Session_finalize(&temp);
    }
    int rc = PMPI_Session_init(info, errhandler, session);
    return rc;
}

int MPI_Group_from_session_pset(MPI_Session session, const char* pset_name, MPI_Group* newgroup)
{
    int rc = PMPI_Group_from_session_pset(session, pset_name, newgroup);
    MPI_Group union_group;
    if (Multicomm::get_instance().get_world_comm() == MPI_COMM_NULL)
        union_group = *newgroup;
    else
    {
        MPI_Group temp, diff;
        int group_size;
        PMPI_Comm_group(Multicomm::get_instance().get_world_comm(), &temp);
        PMPI_Group_union(temp, *newgroup, &union_group);
        PMPI_Group_difference(union_group, temp, &diff);
        PMPI_Group_size(diff, &group_size);
        if (group_size == 0)
            return rc;
    }
    MPI_Comm temp;
    PMPI_Comm_create_from_group(union_group, "Legio_world_group_construction", MPI_INFO_NULL,
                                MPI_ERRORS_RETURN, &temp);
    printf("new_world_comm!\n");
    Multicomm::get_instance().set_world_comm(temp);
    return rc;
}

int MPI_Comm_create_from_group(MPI_Group group,
                               const char* stringtag,
                               MPI_Info info,
                               MPI_Errhandler errhandler,
                               MPI_Comm* newcomm)
{
    int rank, size;
    PMPI_Group_rank(group, &rank);
    PMPI_Group_size(group, &size);

    int rc;
    failure_mtx.lock_shared();
    {
        MPI_Group clean;
        {
            check_group(Multicomm::get_instance().get_world_comm(), group, &clean);
            rc = PMPI_Comm_create_from_group(clean, stringtag, info, errhandler, newcomm);
            MPI_Group_free(&clean);
            legio::report_execution(rc, Multicomm::get_instance().get_world_comm(),
                                    "Comm_create_from_group");
        }
    }
    failure_mtx.unlock_shared();
    if (rc == MPI_SUCCESS && *newcomm != MPI_COMM_NULL)
    {
        Multicomm::get_instance().add_comm(*newcomm);
        return rc;
    }
    else
    {
        return MPI_ERR_PROC_FAILED;
    }
}

int MPI_Session_finalize(MPI_Session* session)
{
    int rc = PMPI_Session_finalize(session);
    return rc;
}