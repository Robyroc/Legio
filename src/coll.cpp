#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "complex_comm.h"


int VERBOSE = 0;

char errstr[MPI_MAX_ERROR_STRING];
int len;
ComplexComm *cur_complex;

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

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    while(1)
    {
        int rc, actual_root, total_size, fake_rank;
        MPI_Comm actual_comm;
        MPI_Comm_size(comm, &total_size);
        MPI_Comm_rank(comm, &fake_rank);
        if(comm == MPI_COMM_WORLD)
        {
            actual_comm = cur_complex->get_comm();
            translate_ranks(root, actual_comm, &actual_root);
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

        PERFORM_GATHER(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, actual_root, actual_comm, total_size, fake_rank, comm);

        gather_handling:
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: gather done (error: %s)\n", rank, size, errstr);
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

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    while(1)
    {
        int rc, actual_root, total_size, fake_rank;
        MPI_Comm actual_comm;
        MPI_Comm_size(comm, &total_size);
        MPI_Comm_rank(comm, &fake_rank);
        if(comm == MPI_COMM_WORLD)
        {
            actual_comm = cur_complex->get_comm();
            translate_ranks(root, actual_comm, &actual_root);
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

        PERFORM_SCATTER(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, actual_root, actual_comm, total_size, fake_rank, comm);

        scatter_handling:
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: scatter done (error: %s)\n", rank, size, errstr);
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