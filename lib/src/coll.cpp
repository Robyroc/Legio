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
        AdvComm* translated = cur_comms->translate_into_complex(comm);
        
        AllToOne first([] (int root, MPI_Comm comm_t) -> int {
            return PMPI_Barrier(comm_t);
        }, false);

        OneToAll second([] (int root, MPI_Comm comm_t) -> int {
            return PMPI_Barrier(comm_t);
        }, false);

        AllToAll func([](MPI_Comm comm_t) -> int {
            return PMPI_Barrier(comm_t);
        }, false, {first, second});

        if(flag)
            rc = translated->perform_operation(func);
        else
            rc = func(comm);

        print_info("barrier", comm, rc);

        if(rc == MPI_SUCCESS || !flag)
            return rc;
        else
            replace_comm(translated);
    }
}

int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_complex(comm);

        OneToAll func([buffer, count, datatype] (int root, MPI_Comm comm_t) -> int {
            if(root == MPI_UNDEFINED)
            {
                HANDLE_BCAST_FAIL(comm_t);
            }
            return PMPI_Bcast(buffer, count, datatype, root, comm_t);
        }, false);

        if(flag)
            rc = translated->perform_operation(func, root);
        else
            rc = func(root, comm);

        print_info("bcast", comm, rc);

        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_Allreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_complex(comm);

        AllToOne first([sendbuf, recvbuf, count, datatype, op] (int root, MPI_Comm comm_t) -> int {
            return PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_t);
        }, false);

        OneToAll second([recvbuf, count, datatype] (int root, MPI_Comm comm_t) -> int {
            return PMPI_Bcast(recvbuf, count, datatype, root, comm_t);
        }, false);

        AllToAll func([sendbuf, recvbuf, count, datatype, op] (MPI_Comm comm_t) -> int {
            return PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_t);
        }, false, {first, second});

        if(flag)
            rc = translated->perform_operation(func);
        else
            rc = func(comm);

        print_info("allreduce", comm, rc);

        if(rc == MPI_SUCCESS || !flag)
            return rc;
        else
            replace_comm(translated);
    }
}

int MPI_Reduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_complex(comm);

        AllToOne func([sendbuf, recvbuf, count, datatype, op] (int root_rank, MPI_Comm comm_t) -> int {
            if(root_rank == MPI_UNDEFINED)
            {
                HANDLE_REDUCE_FAIL(comm_t);
            }
            return PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root_rank, comm_t);
        }, false);

        if(flag)
        {
            rc = translated->perform_operation(func, root);
        }
        else
            rc = func(root, comm);

        print_info("reduce", comm, rc);

        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    while(1)
    {
        int rc, total_size, fake_rank, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_complex(comm);

        MPI_Comm_size(comm, &total_size);
        MPI_Comm_rank(comm, &fake_rank);

        AllToOne func([sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, total_size, fake_rank, comm] (int root_rank, MPI_Comm comm_t) -> int {
            int rc;
            if(root_rank == MPI_UNDEFINED)
            {
                HANDLE_GATHER_FAIL(comm_t);
            }
            PERFORM_GATHER(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root_rank, comm_t, total_size, fake_rank, comm);
            return rc;
        }, true);

        if(flag)
        {
            rc = translated->perform_operation(func, root);
        }
        else
        {
            rc =func(root, comm);
        }

        print_info("gather", comm, rc);

        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    while(1)
    {
        int rc, total_size, fake_rank, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_complex(comm);
        MPI_Comm_size(comm, &total_size);
        MPI_Comm_rank(comm, &fake_rank);

        OneToAll func([sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, total_size, fake_rank, comm] (int root_rank, MPI_Comm comm_t) -> int {
            int rc;
            if(root_rank == MPI_UNDEFINED)
            {
                HANDLE_SCATTER_FAIL(comm_t);
            }
            PERFORM_SCATTER(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root_rank, comm_t, total_size, fake_rank, comm);
            return rc;
        }, true);

        if(flag)
        {
            rc = translated->perform_operation(func, root);
        }
        else
        {
            rc = func(root, comm);
        }

        print_info("scatter", comm, rc);

        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_Scan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_complex(comm);

        AllToOne first([](int, MPI_Comm) -> int {return MPI_SUCCESS;}, false);
        OneToAll second([sendbuf, recvbuf, count, datatype, op] (int, MPI_Comm comm_t) -> int {
            return PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm_t);
        }, false);


        AllToAll func([sendbuf, recvbuf, count, datatype, op] (MPI_Comm comm_t) -> int {
            return PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm_t);
        }, true, {first, second});

        if(flag)
            rc = translated->perform_operation(func);
        else
            rc = func(comm);
        
        print_info("scan", comm, rc);

        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}