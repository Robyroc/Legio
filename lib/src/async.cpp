#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "complex_comm.h"
#include "multicomm.h"
#include <string.h>
#include <stdlib.h>

extern Multicomm *cur_comms;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    ComplexComm* translated = cur_comms->translate_into_complex(comm);

    int size;
    MPI_Type_size(datatype, &size);
    void* tempbuf = malloc(size*count);
    memcpy(tempbuf, buf, size*count);
    std::function<int(MPI_Comm, MPI_Request*)> func;
    if(flag)
    {
        int dest_rank;
        translate_ranks(dest, translated, &dest_rank);
        if(dest_rank == MPI_UNDEFINED)
        {
            HANDLE_SEND_FAIL(cur_complex->get_comm());
        }
        func = [tempbuf, count, datatype, dest, tag, comm] (MPI_Comm actual, MPI_Request* request) -> int {
            MPI_Group old_group, new_group;
            int new_rank;
            MPI_Comm_group(comm, &old_group);
            MPI_Comm_group(actual, &new_group);
            MPI_Group_translate_ranks(old_group, 1, &dest, new_group, &new_rank);
            if(new_rank == MPI_UNDEFINED)
                return MPI_ERR_PROC_FAILED;
            else return PMPI_Isend(tempbuf, count, datatype, new_rank, tag, actual, request);
        };
        rc = PMPI_Isend(buf, count, datatype, dest_rank, tag, translated->get_comm(), request);
    }
    else
        rc = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
    send_handling:
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: isend done (error: %s)\n", rank, size, errstr);
    }
    if(!flag)
        return rc;
    else if(rc == MPI_SUCCESS)
        bool result = cur_comms->add_structure(translated, *request, func);
    free(tempbuf);
    return rc;
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request * request)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    ComplexComm* translated = cur_comms->translate_into_complex(comm);
    std::function<int(MPI_Comm, MPI_Request*)> func;
    if(flag)
    {
        int source_rank;
        translate_ranks(source, translated, &source_rank);
        if(source_rank == MPI_UNDEFINED)
        {
            HANDLE_RECV_FAIL(cur_complex->get_comm());
        }
        func = [buf, count, datatype, source, tag, comm] (MPI_Comm actual, MPI_Request* request) -> int {
            MPI_Group old_group, new_group;
            int new_rank;
            MPI_Comm_group(comm, &old_group);
            MPI_Comm_group(actual, &new_group);
            MPI_Group_translate_ranks(old_group, 1, &source, new_group, &new_rank);
            if(new_rank == MPI_UNDEFINED)
                return MPI_ERR_PROC_FAILED;
            else return PMPI_Irecv(buf, count, datatype, new_rank, tag, actual, request);
        };
        rc = PMPI_Irecv(buf, count, datatype, source_rank, tag, translated->get_comm(), request);
    }
    else
        rc = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
    recv_handling:
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: irecv done (error: %s)\n", rank, size, errstr);
    }
    if(!flag)
        return rc;
    else if(rc == MPI_SUCCESS)
        bool result = cur_comms->add_structure(translated, *request, func);
    return rc;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
    int rc;
    MPI_Request old = *request;
    ComplexComm* comm = cur_comms->get_complex_from_structure(*request);
    if(comm != NULL)
    {
        MPI_Request translated = comm->translate_structure(*request);
        rc = PMPI_Wait(&translated, status);
    }
    else
        rc = PMPI_Wait(request, status);
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: wait done (error: %s)\n", rank, size, errstr);
    }
    cur_comms->remove_request(request);
    return rc;
}

int MPI_Test(MPI_Request* request, int *flag, MPI_Status *status)
{
    int rc;
    ComplexComm* comm = cur_comms->get_complex_from_structure(*request);
    if(comm != NULL)
    {
        MPI_Request translated = comm->translate_structure(*request);
        rc = PMPI_Test(&translated, flag, status);
    }
    else
        rc = PMPI_Test(request, flag, status);
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: test done (error: %s)\n", rank, size, errstr);
    }
    if(*flag)
    {
        cur_comms->remove_request(request);
    }
    return rc;
}

int MPI_Request_free(MPI_Request *request)
{
    cur_comms->remove_request(request);
    return PMPI_Request_free(request);
}