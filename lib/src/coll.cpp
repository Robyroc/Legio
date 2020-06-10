#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "adv_comm.h"
#include "multicomm.h"
#include "operations.h"

extern Multicomm *cur_comms;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

int MPI_Barrier(MPI_Comm comm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_adv(comm);
        
        AllToOne first([] (int root, MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_Barrier(comm_t);
            if(rc != MPI_SUCCESS)
                replace_comm(adv);
            return rc;
        }, false);

        OneToAll second([] (int root, MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_Barrier(comm_t);
            if(rc != MPI_SUCCESS)
                replace_comm(adv);
            return rc;
        }, false);

        AllToAll func([](MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_Barrier(comm_t);
            if(rc != MPI_SUCCESS)
                replace_comm(adv);
            return rc;
        }, false, {first, second});

        rc = translated->perform_operation(func);

        print_info("barrier", comm, rc);

        if(!flag)
            delete translated;

        if(rc == MPI_SUCCESS || !flag)
            return rc;
    }
}

int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_adv(comm);

        OneToAll func([buffer, count, datatype] (int root, MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            if(root == MPI_UNDEFINED)
            {
                HANDLE_BCAST_FAIL(comm_t);
            }
            rc = PMPI_Bcast(buffer, count, datatype, root, comm_t);
            agree_and_eventually_replace(&rc, adv);
            return rc;
        }, false);

        rc = translated->perform_operation(func, root);

        print_info("bcast", comm, rc);

        if(!flag)
            delete translated;

        if(!flag || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_Allreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_adv(comm);

        AllToOne first([sendbuf, recvbuf, count, datatype, op] (int root, MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_t);
            agree_and_eventually_replace(&rc, adv);
            return rc;
        }, false);

        OneToAll second([recvbuf, count, datatype] (int root, MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_Bcast(recvbuf, count, datatype, root, comm_t);
            agree_and_eventually_replace(&rc, adv);
            return rc;
        }, false);

        AllToAll func([sendbuf, recvbuf, count, datatype, op] (MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_t);
            if(rc != MPI_SUCCESS)
                replace_comm(adv);
            return rc;
        }, false, {first, second});

        rc = translated->perform_operation(func);

        print_info("allreduce", comm, rc);

        if(!flag)
            delete translated;

        if(!flag || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_Reduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_adv(comm);

        AllToOne func([sendbuf, recvbuf, count, datatype, op] (int root_rank, MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            if(root_rank == MPI_UNDEFINED)
            {
                HANDLE_REDUCE_FAIL(comm_t);
            }
            rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root_rank, comm_t);
            agree_and_eventually_replace(&rc, adv);
            return rc;
        }, false);

        rc = translated->perform_operation(func, root);

        print_info("reduce", comm, rc);

        if(!flag)
            delete translated;

        if(!flag || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    while(1)
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
            agree_and_eventually_replace(&rc, adv);
            return rc;
        }, true);

        rc = translated->perform_operation(func, root);

        print_info("gather", comm, rc);

        if(!flag)
            delete translated;

        if(!flag || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    while(1)
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
            agree_and_eventually_replace(&rc, adv);
            return rc;
        }, true);

        rc = translated->perform_operation(func, root);

        print_info("scatter", comm, rc);

        if(!flag)
            delete translated;

        if(!flag || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_Scan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_adv(comm);

        AllToOne first([](int, MPI_Comm, AdvComm*) -> int {return MPI_SUCCESS;}, false);
        OneToAll second([sendbuf, recvbuf, count, datatype, op] (int, MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm_t);
            agree_and_eventually_replace(&rc, adv);
            return rc;
        }, false);


        AllToAll func([sendbuf, recvbuf, count, datatype, op] (MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm_t);
            agree_and_eventually_replace(&rc, adv);
            return rc;
        }, true, {first, second});

        rc = translated->perform_operation(func);
        
        print_info("scan", comm, rc);

        if(!flag)
            delete translated;
        
        if(!flag || rc == MPI_SUCCESS)
            return rc;
    }
}