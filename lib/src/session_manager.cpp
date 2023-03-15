#include "session_manager.hpp"
#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include "complex_comm.hpp"
#include "mpi.h"

using namespace legio;

MPI_Comm SessionManager::get_horizon_comm(MPI_Group group)
{
    const std::lock_guard<std::mutex> lock(horizon_lock);
    for (auto horizon : horizon_comms)
    {
        MPI_Group horizon_group, diff;
        int size;
        PMPI_Comm_group(horizon, &horizon_group);
        PMPI_Group_difference(group, horizon_group, &diff);
        PMPI_Group_size(diff, &size);
        PMPI_Group_free(&horizon_group);
        PMPI_Group_free(&diff);
        if (!size)
            return horizon;
    }
    return MPI_COMM_NULL;
}

void SessionManager::add_horizon_comm(MPI_Comm comm)
{
    const std::lock_guard<std::mutex> lock(horizon_lock);
    std::list<MPI_Comm> new_horizon;
    for (std::list<MPI_Comm>::iterator horizon = horizon_comms.begin();
         horizon != horizon_comms.end(); horizon++)
    {
        MPI_Group horizon_group, comm_group, diff_hc, diff_ch;
        int size_hc, size_ch;
        PMPI_Comm_group(*horizon, &horizon_group);
        PMPI_Comm_group(comm, &comm_group);
        PMPI_Group_difference(horizon_group, comm_group, &diff_hc);
        PMPI_Group_difference(comm_group, horizon_group, &diff_ch);
        PMPI_Group_size(diff_hc, &size_hc);
        PMPI_Group_size(diff_ch, &size_ch);
        PMPI_Group_free(&horizon_group);
        PMPI_Group_free(&comm_group);
        PMPI_Group_free(&diff_hc);
        PMPI_Group_free(&diff_ch);
        if (!size_ch)
        {
            // Group is contained by the horizon
            new_horizon.splice(new_horizon.end(), horizon_comms, horizon, horizon_comms.end());
            horizon_comms = new_horizon;
            return;
        }
        if (size_hc)
        {
            // Horizon is not contained by group
            new_horizon.push_back(*horizon);
        }
    }
    new_horizon.push_back(comm);
    horizon_comms = new_horizon;
    return;
}

void SessionManager::add_pending_session(MPI_Session session)
{
    std::unique_lock<std::mutex> lock(session_lock);
    pending.push_back(session);
}
void SessionManager::add_open_session()
{
    std::unique_lock<std::mutex> lock(session_lock);
    open_sessions++;
}
void SessionManager::close_session()
{
    std::unique_lock<std::mutex> lock(session_lock);
    open_sessions--;
    if (open_sessions == 0)
    {
        for (auto session : pending)
            PMPI_Session_finalize(&session);
    }
}

void SessionManager::initialize()
{
    std::unique_lock<std::mutex> lock(init_lock);
    initialized = true;
}

const bool SessionManager::is_initialized()
{
    std::unique_lock<std::mutex> lock(init_lock);
    return initialized;
}