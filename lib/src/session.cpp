#include <signal.h>
#include <future>
#include <shared_mutex>
#include <thread>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "context.hpp"
#include "intercomm_utils.hpp"
#include "log.hpp"
#include "mpi.h"
#include "restart_routines.hpp"

extern std::shared_timed_mutex failure_mtx;
using namespace legio;

#if WITH_SESSION
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
    if (!Context::get().s_manager.is_initialized())
    {
        MPI_Session temp;
        int flag2;
        MPI_Group group;
        MPI_Info tinfo;
        if constexpr (BuildOptions::session_thread)
        {
            PMPI_Info_create(&tinfo);
            PMPI_Info_set(tinfo, "mpi_thread_support_level", "MPI_THREAD_MULTIPLE");
            PMPI_Session_init(tinfo, MPI_ERRORS_RETURN, &temp);
            PMPI_Info_free(&tinfo);
        }
        else
            PMPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &temp);
        PMPI_Group_from_session_pset(temp, "mpi://WORLD", &group);
        Context::get().s_manager.initialize();
        if constexpr (BuildOptions::session_thread)
        {
            std::future<int>* hThread =
                new std::future<int>(std::async(std::launch::async, [group] {
                    MPI_Comm temp;
                    int rc = PMPI_Comm_create_from_group(group, "Legio_horizon_construction",
                                                         MPI_INFO_NULL, MPI_ERRORS_RETURN, &temp);
                    Context::get().s_manager.add_horizon_comm(temp);
                    return rc;
                }));
            if (hThread->wait_for(std::chrono::seconds(5)) == std::future_status::timeout)
            {
                printf("Not all processes reachable during first init, aborting\n");
                exit(-1);
            }
        }
        PMPI_Group_free(&group);
        Context::get().s_manager.add_pending_session(temp);
    }
    int rc = PMPI_Session_init(info, errhandler, session);
    Context::get().s_manager.add_open_session();
    return rc;
}

int MPI_Comm_create_from_group(MPI_Group group,
                               const char* stringtag,
                               MPI_Info info,
                               MPI_Errhandler errhandler,
                               MPI_Comm* newcomm)
{
    int rc;
    failure_mtx.lock_shared();
    {
        MPI_Group clean;
        MPI_Comm horizon = Context::get().s_manager.get_horizon_comm(group);
        if (horizon != MPI_COMM_NULL)
            check_group(horizon, group, &clean);
        else
        {
            legio::log("Executing MPI_Comm_create_from_group in unsafe mode, it can deadlock",
                       LogLevel::errors_and_info);
            clean = group;
        }
        rc = PMPI_Comm_create_from_group(clean, stringtag, info, errhandler, newcomm);
        if (horizon != MPI_COMM_NULL)
            legio::report_execution(rc, horizon, "Comm_create_from_group");
        else
        {
            MPI_Comm temp;
            PMPI_Comm_dup(*newcomm, &temp);
            Context::get().s_manager.add_horizon_comm(temp);
            legio::report_execution(rc, temp, "Comm_create_from_group");
        }
    }
    failure_mtx.unlock_shared();
    if (rc == MPI_SUCCESS && *newcomm != MPI_COMM_NULL)
    {
        Context::get().m_comm.add_comm(*newcomm);
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
    Context::get().s_manager.close_session();
    return rc;
}

#endif