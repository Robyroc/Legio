#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "complex_comm.h"
#include "multicomm.h"
#include <shared_mutex>

extern Multicomm *cur_comms;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

extern std::shared_timed_mutex failure_mtx;

int any_recv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status*);

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    std::shared_lock<std::shared_timed_mutex> lock(failure_mtx);

    int i, rc, flag;
    cur_comms->part_of(comm, &flag);
    ComplexComm* translated = cur_comms->translate_into_complex(comm);
    for (i = 0; i < NUM_RETRY; i++)
    {
        if(flag)
        {
            int dest_rank;
            translate_ranks(dest, translated, &dest_rank);
            if(dest_rank == MPI_UNDEFINED)
            {
                HANDLE_SEND_FAIL(cur_complex->get_comm());
            }
            rc = PMPI_Send(buf, count, datatype, dest_rank, tag, translated->get_comm());
        }
        else
            rc = PMPI_Send(buf, count, datatype, dest, tag, comm);
        send_handling:
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: send done (error: %s)\n", rank, size, errstr);
        }
        if(rc == MPI_SUCCESS)
            return rc;
    }
    return rc;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    std::shared_lock<std::shared_timed_mutex> lock(failure_mtx);

    if(source == MPI_ANY_SOURCE)
        return any_recv(buf, count, datatype, source, tag, comm, status);

    int rc, flag;
    cur_comms->part_of(comm, &flag);
    ComplexComm* translated = cur_comms->translate_into_complex(comm);
    if(flag)
    {
        int source_rank;
        translate_ranks(source, translated, &source_rank);
        if(source_rank == MPI_UNDEFINED)
        {
            HANDLE_RECV_FAIL(cur_complex->get_comm());
        }
        rc = PMPI_Recv(buf, count, datatype, source_rank, tag, translated->get_comm(), status);
    }
    else
        rc = PMPI_Recv(buf, count, datatype, source, tag, translated->get_comm(), status);
    recv_handling:
    if(VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: recv done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
    std::shared_lock<std::shared_timed_mutex> lock(failure_mtx);

    int rc, flag;
    cur_comms->part_of(comm, &flag);
    ComplexComm* translated = cur_comms->translate_into_complex(comm);
    if(flag)
    {
        int source_rank, dest_rank;
        translate_ranks(source, translated, &source_rank);
        translate_ranks(dest, translated, &dest_rank);
        if(source_rank == MPI_UNDEFINED)
        {
            HANDLE_RECV_FAIL(cur_complex->get_comm());
        }
        rc = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest_rank, sendtag, recvbuf, recvcount, recvtype, source_rank, recvtag, translated->get_comm(), status);
    }
    else
        rc = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status);
    recv_handling:
    if(VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: sendrecv done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int any_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    ComplexComm* translated = cur_comms->translate_into_complex(comm);
    if(flag)
    {
        rc = PMPI_Recv(buf, count, datatype, source, tag, translated->get_comm(), status);
    }
    else
        rc = PMPI_Recv(buf, count, datatype, source, tag, translated->get_comm(), status);
    if(VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: recv done (error: %s)\n", rank, size, errstr);
    }
    if(rc != MPI_SUCCESS)
    {
        /*
        int eclass;
        MPI_Error_class(rc, &eclass);
        if( MPIX_ERR_PROC_FAILED != eclass ) 
        {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        */
        MPIX_Comm_failure_ack(translated->get_comm());
    }
    return rc;
}