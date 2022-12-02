#include <mpi.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shared_mutex>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "configuration.hpp"
#include "mpi-ext.h"
#include "multicomm.hpp"

extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

extern std::shared_timed_mutex failure_mtx;

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
    int flag = Multicomm::get_instance().part_of(comm);
    MPI_Type_size(datatype, &size);
    void* tempbuf = malloc(size * count);
    memcpy(tempbuf, buf, size * count);
    std::function<int(MPI_Comm, MPI_Request*)> func;
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
        int dest_rank;
        dest_rank = Multicomm::get_instance().translate_ranks(dest, translated);
        if (dest_rank == MPI_UNDEFINED)
        {
            HANDLE_SEND_FAIL(translated.get_comm());
        }
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
    else
        rc = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
send_handling:
    failure_mtx.unlock_shared();
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: isend done (error: %s)\n", rank, size, errstr);
    }
    if (!flag)
        return rc;
    else if (rc == MPI_SUCCESS)
        bool result = Multicomm::get_instance().add_structure(
            Multicomm::get_instance().translate_into_complex(comm), *request, func);
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
    bool flag = Multicomm::get_instance().part_of(comm);
    std::function<int(MPI_Comm, MPI_Request*)> func;
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
        int source_rank = Multicomm::get_instance().translate_ranks(source, translated);
        if (source_rank == MPI_UNDEFINED)
        {
            HANDLE_RECV_FAIL(translated.get_comm());
        }
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
    else
        rc = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
recv_handling:
    failure_mtx.unlock_shared();
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: irecv done (error: %s)\n", rank, size, errstr);
    }
    if (!flag)
        return rc;
    else if (rc == MPI_SUCCESS)
        bool result = Multicomm::get_instance().add_structure(
            Multicomm::get_instance().translate_into_complex(comm), *request, func);
    return rc;
}

int MPI_Wait(MPI_Request* request, MPI_Status* status)
{
    int rc;
    MPI_Request old = *request;
    bool flag = Multicomm::get_instance().part_of(*request);
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& comm = Multicomm::get_instance().get_complex_from_structure(*request);
        MPI_Request translated = comm.translate_structure(*request);
        rc = PMPI_Wait(&translated, status);
    }
    else
        rc = PMPI_Wait(request, status);

    failure_mtx.unlock_shared();
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: wait done (error: %s)\n", rank, size, errstr);
    }

    Multicomm::get_instance().remove_structure(request);
    return rc;
}

int MPI_Test(MPI_Request* request, int* flag, MPI_Status* status)
{
    int rc;
    bool part = Multicomm::get_instance().part_of(*request);
    failure_mtx.lock_shared();
    if (part)
    {
        ComplexComm& comm = Multicomm::get_instance().get_complex_from_structure(*request);
        MPI_Request translated = comm.translate_structure(*request);
        rc = PMPI_Test(&translated, flag, status);
    }
    else
        rc = PMPI_Test(request, flag, status);
    failure_mtx.unlock_shared();
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: test done (error: %s)\n", rank, size, errstr);
    }
    if (*flag)
    {
        Multicomm::get_instance().remove_structure(request);
    }
    return rc;
}

int MPI_Request_free(MPI_Request* request)
{
    Multicomm::get_instance().remove_structure(request);
    return PMPI_Request_free(request);
}
