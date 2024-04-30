#include <mpi.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shared_mutex>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "config.hpp"
#include "context.hpp"
#include "log.hpp"
#include "mpi-ext.h"

extern std::shared_timed_mutex failure_mtx;
using namespace legio;

int MPI_Isend(const void* buf,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm,
              MPI_Request* request)
{
    int rc;

    int size;
    int flag = Context::get().m_comm.part_of(comm);
    MPI_Type_size(datatype, &size);
    void* tempbuf = malloc(size * count);
    memcpy(tempbuf, buf, size * count);
    std::function<int(MPI_Comm, MPI_Request*)> func;
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
        int dest_rank;
        dest_rank = translate_ranks(dest, translated);
        if (dest_rank == MPI_UNDEFINED)
        {
            if constexpr (BuildOptions::send_resiliency)
                rc = MPI_SUCCESS;
            else
            {
                legio::log("##### Isend failed, stopping a node", LogLevel::errors_only);
                raise(SIGINT);
            }
        }
        else
        {
            func = [tempbuf, count, datatype, dest, tag, comm](MPI_Comm actual,
                                                               MPI_Request* request) -> int {
                MPI_Group old_group, new_group;
                int new_rank;
                MPI_Comm_group(comm, &old_group);
                MPI_Comm_group(actual, &new_group);
                MPI_Group_translate_ranks(old_group, 1, &dest, new_group, &new_rank);
                if (new_rank == MPI_UNDEFINED)
                    return MPI_ERR_PROC_FAILED;
                else
                    return PMPI_Isend(tempbuf, count, datatype, new_rank, tag, actual, request);
            };
            rc = PMPI_Isend(buf, count, datatype, dest_rank, tag, translated.get_comm(), request);
        }
    }
    else
        rc = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
    failure_mtx.unlock_shared();
    legio::report_execution(rc, comm, "Isend");
    if (!flag)
        return rc;
    else if (rc == MPI_SUCCESS)
        bool result = Context::get().m_comm.add_structure(
            Context::get().m_comm.translate_into_complex(comm), *request, func);
    free(tempbuf);
    return rc;
}

int MPI_Irecv(void* buf,
              int count,
              MPI_Datatype datatype,
              int source,
              int tag,
              MPI_Comm comm,
              MPI_Request* request)
{
    int rc;
    bool flag = Context::get().m_comm.part_of(comm);
    std::function<int(MPI_Comm, MPI_Request*)> func;
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
        int source_rank = translate_ranks(source, translated);
        if (source_rank == MPI_UNDEFINED)
        {
            if constexpr (BuildOptions::recv_resiliency)
                rc = MPI_SUCCESS;
            else
            {
                legio::log("##### Irecv failed, stopping a node", LogLevel::errors_only);
                raise(SIGINT);
            }
        }
        else
        {
            func = [buf, count, datatype, source, tag, comm](MPI_Comm actual,
                                                             MPI_Request* request) -> int {
                MPI_Group old_group, new_group;
                int new_rank;
                MPI_Comm_group(comm, &old_group);
                MPI_Comm_group(actual, &new_group);
                MPI_Group_translate_ranks(old_group, 1, &source, new_group, &new_rank);
                if (new_rank == MPI_UNDEFINED)
                    return MPI_ERR_PROC_FAILED;
                else
                    return PMPI_Irecv(buf, count, datatype, new_rank, tag, actual, request);
            };
            rc = PMPI_Irecv(buf, count, datatype, source_rank, tag, translated.get_comm(), request);
        }
    }
    else
        rc = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
    failure_mtx.unlock_shared();
    legio::report_execution(rc, comm, "Irecv");
    if (!flag)
        return rc;
    else if (rc == MPI_SUCCESS)
        bool result = Context::get().m_comm.add_structure(
            Context::get().m_comm.translate_into_complex(comm), *request, func);
    return rc;
}

int MPI_Wait(MPI_Request* request, MPI_Status* status)
{
    int rc;
    MPI_Request old = *request;
    bool flag = Context::get().m_comm.part_of(*request);
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& comm = Context::get().m_comm.get_complex_from_structure(*request);
        MPI_Request translated = comm.translate_structure(*request);
        rc = PMPI_Wait(&translated, status);
    }
    else
        rc = PMPI_Wait(request, status);

    failure_mtx.unlock_shared();
    legio::report_execution(rc, MPI_COMM_WORLD, "Wait");

    Context::get().m_comm.remove_structure(request);
    return rc;
}

int MPI_Test(MPI_Request* request, int* flag, MPI_Status* status)
{
    int rc;
    bool part = Context::get().m_comm.part_of(*request);
    failure_mtx.lock_shared();
    if (part)
    {
        ComplexComm& comm = Context::get().m_comm.get_complex_from_structure(*request);
        MPI_Request translated = comm.translate_structure(*request);
        rc = PMPI_Test(&translated, flag, status);
    }
    else
        rc = PMPI_Test(request, flag, status);
    failure_mtx.unlock_shared();
    legio::report_execution(rc, MPI_COMM_WORLD, "Test");
    if (*flag)
    {
        Context::get().m_comm.remove_structure(request);
    }
    return rc;
}

int MPI_Request_free(MPI_Request* request)
{
    Context::get().m_comm.remove_structure(request);
    return PMPI_Request_free(request);
}


// to be checked
int MPI_Waitall(int count, MPI_Request *array_of_requests, MPI_Status *array_of_statuses) {
    int rc;
    MPI_Request *request_ptr = array_of_requests;
    MPI_Status *status_ptr = array_of_statuses;
    for (int i = 0; i < count; ++i) {
        rc = MPI_Wait(request_ptr, status_ptr);
        if (rc != MPI_SUCCESS) {
            // Error handling
            return rc;
        }
        ++request_ptr;
        ++status_ptr;
    }
    return rc;
}

