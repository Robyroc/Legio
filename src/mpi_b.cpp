#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "complex_comm.h"


#define VERBOSE 1

char errstr[MPI_MAX_ERROR_STRING];
int len, rc;
ComplexComm *cur_complex;

int any_recv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status*);

int MPI_Init(int* argc, char *** argv)
{
    MPI_Comm temp;
    int rc = PMPI_Init(argc, argv);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    PMPI_Comm_dup(MPI_COMM_WORLD, &temp);
    cur_complex = new ComplexComm(temp);
    MPI_Comm_set_errhandler(cur_complex->get_comm(), MPI_ERRORS_RETURN);
    return rc;
}

int MPI_Barrier(MPI_Comm comm)
{
    while(1)
    {
        int rc;
        if(comm == MPI_COMM_WORLD)
            rc = PMPI_Barrier(cur_complex->get_comm());
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
            replace_comm(cur_complex);
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
            translate_ranks(root, cur_complex->get_comm(), &root_rank);
            if(root_rank == MPI_UNDEFINED)
            {
                HANDLE_BCAST_FAIL(cur_complex->get_comm());
            }
            rc = PMPI_Bcast(buffer, count, datatype, root_rank, cur_complex->get_comm());
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
            agree_and_eventually_replace(&rc, cur_complex);
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
            translate_ranks(dest, cur_complex->get_comm(), &dest_rank);
            if(dest_rank == MPI_UNDEFINED)
            {
                HANDLE_SEND_FAIL(cur_complex->get_comm());
            }
            rc = PMPI_Send(buf, count, datatype, dest_rank, tag, cur_complex->get_comm());
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
        translate_ranks(source, cur_complex->get_comm(), &source_rank);
        if(source_rank == MPI_UNDEFINED)
        {
            HANDLE_RECV_FAIL(cur_complex->get_comm());
        }
        rc = PMPI_Recv(buf, count, datatype, source_rank, tag, cur_complex->get_comm(), status);
    }
    else
        rc = PMPI_Recv(buf, count, datatype, source, tag, cur_complex->get_comm(), status);
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

int MPI_Allreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    while(1)
    {
        int rc;
        if(comm == MPI_COMM_WORLD)
            rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, cur_complex->get_comm());
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
            replace_comm(cur_complex);
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
            translate_ranks(root, cur_complex->get_comm(), &root_rank);
            if(root_rank == MPI_UNDEFINED)
            {
                HANDLE_REDUCE_FAIL(cur_complex->get_comm());
            }
            rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root_rank, cur_complex->get_comm());
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
            agree_and_eventually_replace(&rc, cur_complex);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_Win_create(void* base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    while(1)
    {
        int rc;
        if(comm == MPI_COMM_WORLD)
            rc = PMPI_Win_create(base, size, disp_unit, info, cur_complex->get_comm(), win);
        else
            rc = PMPI_Win_create(base, size, disp_unit, info, comm, win);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: win created (error: %s)\n", rank, size, errstr);
        }
        if(comm != MPI_COMM_WORLD)
            return rc;
        else if(rc == MPI_SUCCESS)
        {
            cur_complex->add_window(base, size, disp_unit, info, *win);
            return rc;
        }
        else
            replace_comm(cur_complex);
    }
}

int MPI_Win_free(MPI_Win *win)
{
    cur_complex->remove_window(*win);
    return PMPI_Win_free(win);
}

int MPI_Win_fence(int assert, MPI_Win win)
{
    while(1)
    {
        int rc, flag;
        cur_complex->check_global(win, &flag);
        MPI_Win translated = cur_complex->translate_win(win);
        rc = PMPI_Win_fence(assert, translated);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: fence done (error: %s)\n", rank, size, errstr);
        }
        if(rc == MPI_SUCCESS || !flag)
            return rc;
        else
            replace_comm(cur_complex);
    }
}

int MPI_Get(void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
    int rc, flag;
    cur_complex->check_global(win, &flag);
    if(flag)
    {
        MPI_Win translated = cur_complex->translate_win(win);
        int new_rank;
        translate_ranks(target_rank, cur_complex->get_comm(), &new_rank);
        if(new_rank == MPI_UNDEFINED)
        {
            HANDLE_GET_FAIL(cur_complex->get_comm());
        }
        rc = PMPI_Get(origin_addr, origin_count, origin_datatype, new_rank, target_disp, target_count, target_datatype, translated);
    }
    else
        rc = PMPI_Get(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
    get_handling:
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: get done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_Put(void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
    int rc, flag;
    cur_complex->check_global(win, &flag);
    if(flag)
    {
        MPI_Win translated = cur_complex->translate_win(win);
        int new_rank;
        translate_ranks(target_rank, cur_complex->get_comm(), &new_rank);
        if(new_rank == MPI_UNDEFINED)
        {
            HANDLE_PUT_FAIL(cur_complex->get_comm());
        }
        rc = PMPI_Put(origin_addr, origin_count, origin_datatype, new_rank, target_disp, target_count, target_datatype, translated);
    }
    else
        rc = PMPI_Put(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
    put_handling:
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: put done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int any_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    int rc;
    if(comm == MPI_COMM_WORLD)
    {
        rc = PMPI_Recv(buf, count, datatype, source, tag, cur_complex->get_comm(), status);
    }
    else
        rc = PMPI_Recv(buf, count, datatype, source, tag, cur_complex->get_comm(), status);
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