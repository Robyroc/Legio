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

int MPI_Barrier(MPI_Comm comm)
{
    int rank;
    MPI_Comm_rank(comm, &rank);
    while(1)
    {
        int rc, flag;
        int rank, size;
        cur_comms->part_of(comm, &flag);
        ComplexComm* translated = cur_comms->translate_into_complex(comm);
        failure_mtx.lock_shared(); 
        if(flag) {
            int actual_size_comm, rankk;
            MPI_Comm_size(translated->get_comm(), &actual_size_comm);
            MPI_Comm_rank(comm, &rankk);
            printf("\nBarrier with actual size %d from rank %d\n", actual_size_comm, rankk); fflush(stdout);
            rc = PMPI_Barrier(translated->get_comm());
        }
        else  {
                rc = PMPI_Barrier(comm);

        }
        failure_mtx.unlock_shared(); 

        if (VERBOSE)
        {
            MPI_Comm_size(comm, &size);
            MPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);            
            printf("Rank %d / %d: barrier done (error: %s)\n", rank, size, errstr); fflush(stdout);
        }
        
        if(rc == MPI_SUCCESS || !flag) {
            return rc;
        }
        else {
            replace_comm(translated);
        }
    }
}

int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        ComplexComm* translated = cur_comms->translate_into_complex(comm);
        failure_mtx.lock_shared(); 
        if(flag)
        {
            int root_rank;
            cur_comms->translate_ranks(root, translated, &root_rank);
            if(root_rank == MPI_UNDEFINED)
            {
                HANDLE_BCAST_FAIL(translated->get_comm());
            }
            rc = PMPI_Bcast(buffer, count, datatype, root_rank, translated->get_comm());
        }
        else
            rc = PMPI_Bcast(buffer, count, datatype, root, comm);
        bcast_handling:
        failure_mtx.unlock_shared(); 
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: bcast done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            // if(rc == MPI_SUCCESS)
            //     return rc;
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
        ComplexComm* translated = cur_comms->translate_into_complex(comm);
        failure_mtx.lock_shared(); 
        if(flag)
            rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, translated->get_comm());
        else
            rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
        failure_mtx.unlock_shared(); 
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: allreduce done (error: %s)\n", rank, size, errstr);
        }
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
        ComplexComm* translated = cur_comms->translate_into_complex(comm);
        failure_mtx.lock_shared(); 
        if(flag)
        {
            int root_rank;
            cur_comms->translate_ranks(root, translated, &root_rank);
            if(root_rank == MPI_UNDEFINED)
            {
                HANDLE_REDUCE_FAIL(translated.get_comm());
            }
            rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root_rank, translated->get_comm());
        }
        else
            rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
        reduce_handling:
        failure_mtx.unlock_shared(); 
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: reduce done (error: %s)\n", rank, size, errstr);
        }
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
        int rc, actual_root, total_size, fake_rank, flag;
        cur_comms->part_of(comm, &flag);
        ComplexComm* translated = cur_comms->translate_into_complex(comm);
        MPI_Comm actual_comm;
        MPI_Comm_size(comm, &total_size);
        MPI_Comm_rank(comm, &fake_rank);
        if(flag)
        {
            actual_comm = translated->get_comm();
            cur_comms->translate_ranks(root, translated, &actual_root);
            if(actual_root == MPI_UNDEFINED)
            {
                HANDLE_GATHER_FAIL(actual_comm);
            }
        }
        else
        {
            actual_comm = comm;
            actual_root = root;
        }

        failure_mtx.lock_shared(); 
        PERFORM_GATHER(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, actual_root, actual_comm, total_size, fake_rank, comm);

        gather_handling:
        failure_mtx.unlock_shared(); 
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: gather done (error: %s)\n", rank, size, errstr);
        }
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
        int rc, actual_root, total_size, fake_rank, flag;
        cur_comms->part_of(comm, &flag);
        ComplexComm* translated = cur_comms->translate_into_complex(comm);
        MPI_Comm actual_comm;
        MPI_Comm_size(comm, &total_size);
        MPI_Comm_rank(comm, &fake_rank);
        if(flag)
        {
            actual_comm = translated->get_comm();
            cur_comms->translate_ranks(root, translated, &actual_root);
            if(actual_root == MPI_UNDEFINED)
            {
                HANDLE_SCATTER_FAIL(actual_comm);
            }
        }
        else
        {
            actual_comm = comm;
            actual_root = root;
        }
        failure_mtx.lock_shared(); 
        PERFORM_SCATTER(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, actual_root, actual_comm, total_size, fake_rank, comm);

        scatter_handling:
        failure_mtx.unlock_shared(); 
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: scatter done (error: %s)\n", rank, size, errstr);
        }
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
        ComplexComm* translated = cur_comms->translate_into_complex(comm);
        failure_mtx.lock_shared(); 
        if(flag)
            rc = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, translated->get_comm());
        else
            rc = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
        failure_mtx.unlock_shared(); 
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: scan done (error: %s)\n", rank, size, errstr);
        }
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