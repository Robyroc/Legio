#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "adv_comm.h"
#include "multicomm.h"
#include "operations.h"
#include <string.h>

extern Multicomm *cur_comms;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

int MPI_Barrier(MPI_Comm comm)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);
    
    AllToOne first([] (int root, MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc;
        rc = PMPI_Barrier(comm_t);
        if(rc != MPI_SUCCESS)
            replace_comm(adv, comm_t);
        return rc;
    }, false);

    OneToAll second([] (int root, MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc;
        rc = PMPI_Barrier(comm_t);
        if(rc != MPI_SUCCESS)
            replace_comm(adv, comm_t);
        return rc;
    }, false);

    AllToAll func([](MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc;
        rc = PMPI_Barrier(comm_t);
        if(rc != MPI_SUCCESS)
            replace_comm(adv, comm_t);
        return rc;
    }, false, {first, second});

    rc = translated->perform_operation(func);

    print_info("barrier", comm, rc);

    if(!flag)
        delete translated;

    return rc;
}

int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);

    OneToAll func([buffer, count, datatype] (int root_rank, MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc;
        if(root_rank == MPI_UNDEFINED)
        {
            HANDLE_BCAST_FAIL(comm_t);
        }
        rc = PMPI_Bcast(buffer, count, datatype, root_rank, comm_t);
        agree_and_eventually_replace(&rc, adv, comm_t);
        return rc;
    }, false);

    rc = translated->perform_operation(func, root);

    print_info("bcast", comm, rc);

    if(!flag)
        delete translated;

    return rc;
}

int MPI_Allreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);

    int size;
    MPI_Type_size(datatype, &size);
    void* first_buf = malloc(size*count);
    void* second_buf = malloc(size*count);
    memcpy(first_buf, sendbuf, size*count);

    AllToOne first([first_buf, second_buf, count, datatype, op, size, recvbuf] (int root, MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc;
        //check if recvbuf is null, may be useful to malloc
        rc = PMPI_Reduce(first_buf, second_buf, count, datatype, op, root, comm_t);
        agree_and_eventually_replace(&rc, adv, comm_t);
        memcpy(first_buf, second_buf, size*count);
        memcpy(recvbuf, second_buf, size*count);
        return rc;
    }, false);

    OneToAll second([recvbuf, count, datatype] (int root, MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc;
        rc = PMPI_Bcast(recvbuf, count, datatype, root, comm_t);
        agree_and_eventually_replace(&rc, adv, comm_t);
        return rc;
    }, false);

    AllToAll func([sendbuf, recvbuf, count, datatype, op] (MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc;
        rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_t);
        if(rc != MPI_SUCCESS)
            replace_comm(adv, comm_t);
        return rc;
    }, false, {first, second});

    rc = translated->perform_operation(func);

    print_info("allreduce", comm, rc);

    free(first_buf); free(second_buf);

    if(!flag)
        delete translated;

    return rc;
}

int MPI_Reduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);

    int size;
    MPI_Type_size(datatype, &size);
    void* first_buf = malloc(size*count);
    void* second_buf = malloc(size*count);
    memcpy(first_buf, sendbuf, size*count);

    AllToOne func([first_buf, second_buf, count, datatype, op, size] (int root_rank, MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc;
        if(root_rank == MPI_UNDEFINED)
        {
            HANDLE_REDUCE_FAIL(comm_t);
        }
        rc = PMPI_Reduce(first_buf, second_buf, count, datatype, op, root_rank, comm_t);
        agree_and_eventually_replace(&rc, adv, comm_t);
        memcpy(first_buf, second_buf, size*count);
        return rc;
    }, false);

    rc = translated->perform_operation(func, root);

    memcpy(recvbuf, second_buf, size*count);
    free(first_buf); free(second_buf);

    print_info("reduce", comm, rc);

    if(!flag)
        delete translated;

    return rc;
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int rc, total_size, fake_rank, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);

    MPI_Comm_size(comm, &total_size);
    MPI_Comm_rank(comm, &fake_rank);

    AllToOne func([sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, total_size, fake_rank, comm] (int root_rank, MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc;
        if(root_rank == MPI_UNDEFINED)
        {
            HANDLE_GATHER_FAIL(comm_t);
        }
        PERFORM_GATHER(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root_rank, comm_t, total_size, fake_rank, comm);
        agree_and_eventually_replace(&rc, adv, comm_t);
        return rc;
    }, true);

    rc = translated->perform_operation(func, root);

    print_info("gather", comm, rc);

    if(!flag)
        delete translated;

    return rc;
}

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int rc, total_size, fake_rank, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);
    MPI_Comm_size(comm, &total_size);
    MPI_Comm_rank(comm, &fake_rank);

    OneToAll func([sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, total_size, fake_rank, comm] (int root_rank, MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc;
        if(root_rank == MPI_UNDEFINED)
        {
            HANDLE_SCATTER_FAIL(comm_t);
        }
        PERFORM_SCATTER(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root_rank, comm_t, total_size, fake_rank, comm);
        agree_and_eventually_replace(&rc, adv, comm_t);
        return rc;
    }, true);

    rc = translated->perform_operation(func, root);

    print_info("scatter", comm, rc);

    if(!flag)
        delete translated;

    return rc;
}

int MPI_Scan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_adv(comm);

        LocalOnly func([sendbuf, recvbuf, count, datatype, op] (MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm_t);
            agree_and_eventually_replace(&rc, adv, comm_t);
            return rc;
        }, true);

        rc = translated->perform_operation(func);
        
        print_info("scan", comm, rc);

        if(!flag)
            delete translated;
        
        if(!flag || rc == MPI_SUCCESS)
            return rc;
    }
}