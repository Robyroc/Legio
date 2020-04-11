#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"

#define VERBOSE 1

char errstr[MPI_MAX_ERROR_STRING];
int len, rc;
MPI_Comm cur_comm;

int any_recv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status*);

int MPI_Init(int* argc, char *** argv)
{
    int rc = PMPI_Init(argc, argv);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    PMPI_Comm_dup(MPI_COMM_WORLD, &cur_comm);
    MPI_Comm_set_errhandler(cur_comm, MPI_ERRORS_RETURN);
    return rc;
}

int MPI_Barrier(MPI_Comm comm)
{
    while(1)
    {
        int rc;
        if(comm == MPI_COMM_WORLD)
            rc = PMPI_Barrier(cur_comm);
        else
            rc = PMPI_Barrier(comm);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: barrier done (error: %s)\n", rank, size, errstr);
        }
        if(rc == MPI_SUCCESS || comm != MPI_COMM_WORLD)
            return rc;
        else
            replace_comm(&cur_comm);
    }
}

int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    while(1)
    {
        int rc;
        if(comm == MPI_COMM_WORLD)
        {
            int root_rank;
            translate_ranks(root, cur_comm, &root_rank);
            if(root_rank == MPI_UNDEFINED)
            {
                HANDLE_BCAST_FAIL(cur_comm);
            }
            rc = PMPI_Bcast(buffer, count, datatype, root_rank, cur_comm);
        }
        else
            rc = PMPI_Bcast(buffer, count, datatype, root, comm);
        bcast_handling:
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: bcast done (error: %s)\n", rank, size, errstr);
        }
        if(comm == MPI_COMM_WORLD)
        {
            agree_and_eventually_replace(&rc, &cur_comm);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}


int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int i, rc;
    for(i = 0; i < NUM_RETRY; i++)
    {
        if(comm == MPI_COMM_WORLD)
        {
            int dest_rank;
            translate_ranks(dest, cur_comm, &dest_rank);
            if(dest_rank == MPI_UNDEFINED)
            {
                HANDLE_SEND_FAIL(cur_comm);
            }
            rc = PMPI_Send(buf, count, datatype, dest_rank, tag, cur_comm);
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
    if(source == MPI_ANY_SOURCE)
        return any_recv(buf, count, datatype, source, tag, comm, status);

    int rc;
    if(comm == MPI_COMM_WORLD)
    {
        int source_rank;
        translate_ranks(source, cur_comm, &source_rank);
        if(source_rank == MPI_UNDEFINED)
        {
            HANDLE_RECV_FAIL(cur_comm);
        }
        rc = PMPI_Recv(buf, count, datatype, source_rank, tag, cur_comm, status);
    }
    else
        rc = PMPI_Recv(buf, count, datatype, source, tag, cur_comm, status);
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

int any_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    int rc;
    if(comm == MPI_COMM_WORLD)
    {
        rc = PMPI_Recv(buf, count, datatype, source, tag, cur_comm, status);
    }
    else
        rc = PMPI_Recv(buf, count, datatype, source, tag, cur_comm, status);
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
        int eclass;
        MPI_Error_class(rc, &eclass);
        if( MPIX_ERR_PROC_FAILED != eclass ) 
        {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        MPIX_Comm_failure_ack(MPI_COMM_WORLD);
    }
    return rc;
}

int MPI_Allreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    while(1)
    {
        int rc;
        if(comm == MPI_COMM_WORLD)
            rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, cur_comm);
        else
            rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: allreduce done (error: %s)\n", rank, size, errstr);
        }
        if(rc == MPI_SUCCESS || comm != MPI_COMM_WORLD)
            return rc;
        else
            replace_comm(&cur_comm);
    }
}

int MPI_Reduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    while(1)
    {
        int rc;
        if(comm == MPI_COMM_WORLD)
        {
            int root_rank;
            translate_ranks(root, cur_comm, &root_rank);
            if(root_rank == MPI_UNDEFINED)
            {
                HANDLE_REDUCE_FAIL(cur_comm);
            }
            rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root_rank, cur_comm);
        }
        else
            rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
        reduce_handling:
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: reduce done (error: %s)\n", rank, size, errstr);
        }
        if(comm == MPI_COMM_WORLD)
        {
            agree_and_eventually_replace(&rc, &cur_comm);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}
