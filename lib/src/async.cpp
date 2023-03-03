#include <mpi.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shared_mutex>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "config.hpp"
#include "log.hpp"
#include ULFM_HDR
#include "mpi_structs.hpp"
#include "multicomm.hpp"

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
    int flag = Multicomm::get_instance().part_of(static_cast<Legio_comm>(comm));
    MPI_Type_size(datatype, &size);
    void* tempbuf = malloc(size * count);
    memcpy(tempbuf, buf, size * count);
    std::function<int(Legio_comm, Legio_request*)> func;
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
        int dest_rank;
        dest_rank = Multicomm::get_instance().translate_ranks(dest, translated);
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
            func = [tempbuf, count, datatype, dest, tag, comm](Legio_comm actual,
                                                               Legio_request* request) -> int {
                MPI_Group old_group, new_group;
                MPI_Request temp;
                int new_rank;
                MPI_Comm_group(comm, &old_group);
                MPI_Comm_group(actual, &new_group);
                MPI_Group_translate_ranks(old_group, 1, &dest, new_group, &new_rank);
                if (new_rank == MPI_UNDEFINED)
                    return MPI_ERR_PROC_FAILED;
                else
                {
                    int rc = PMPI_Isend(tempbuf, count, datatype, new_rank, tag, actual, &temp);
                    *request = temp;
                    return rc;
                }
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
    {
        Legio_request temp = *request;
        bool result = Multicomm::get_instance().add_structure(
            Multicomm::get_instance().translate_into_complex(comm), temp, func);
    }
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
    bool flag = Multicomm::get_instance().part_of(static_cast<Legio_comm>(comm));
    std::function<int(Legio_comm, Legio_request*)> func;
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
        int source_rank = Multicomm::get_instance().translate_ranks(source, translated);
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
            func = [buf, count, datatype, source, tag, comm](Legio_comm actual,
                                                             Legio_request* request) -> int {
                MPI_Group old_group, new_group;
                int new_rank;
                MPI_Comm_group(comm, &old_group);
                MPI_Comm_group(actual, &new_group);
                MPI_Group_translate_ranks(old_group, 1, &source, new_group, &new_rank);
                if (new_rank == MPI_UNDEFINED)
                    return MPI_ERR_PROC_FAILED;
                else
                {
                    MPI_Request temp = *request;
                    return PMPI_Irecv(buf, count, datatype, new_rank, tag, actual, &temp);
                    *request = temp;
                }
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
    {
        Legio_request temp = *request;
        bool result = Multicomm::get_instance().add_structure(
            Multicomm::get_instance().translate_into_complex(comm), temp, func);
    }
    return rc;
}

int MPI_Wait(MPI_Request* request, MPI_Status* status)
{
    int rc;
    Legio_request req = *request;
    bool flag = Multicomm::get_instance().part_of(req);
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& comm = Multicomm::get_instance().get_complex_from_structure(req);
        Legio_request transl = comm.translate_structure(req);
        MPI_Request translated = transl;
        rc = PMPI_Wait(&translated, status);
    }
    else
        rc = PMPI_Wait(request, status);

    failure_mtx.unlock_shared();
    legio::report_execution(rc, MPI_COMM_WORLD, "Wait");
    Multicomm::get_instance().remove_structure(&req);
    *request = req;
    return rc;
}

int MPI_Test(MPI_Request* request, int* flag, MPI_Status* status)
{
    int rc;
    Legio_request req = *request;
    bool part = Multicomm::get_instance().part_of(req);
    failure_mtx.lock_shared();
    if (part)
    {
        ComplexComm& comm = Multicomm::get_instance().get_complex_from_structure(req);
        Legio_request transl = comm.translate_structure(req);
        MPI_Request translated = transl;
        rc = PMPI_Test(&translated, flag, status);
    }
    else
        rc = PMPI_Test(request, flag, status);
    failure_mtx.unlock_shared();
    legio::report_execution(rc, MPI_COMM_WORLD, "Test");
    if (*flag)
    {
        Legio_request req = *request;
        Multicomm::get_instance().remove_structure(&req);
        *request = req;
    }
    return rc;
}

int MPI_Request_free(MPI_Request* request)
{
    Legio_request req = *request;
    Multicomm::get_instance().remove_structure(&req);
    *request = req;
    return PMPI_Request_free(request);
}
