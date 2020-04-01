#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"

#define VERBOSE 1

char errstr[MPI_MAX_ERROR_STRING];
int len, rc;
MPI_Comm cur_comm;

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
                raise(SIGINT);
                //handle missing root, user configured
            }
            rc = PMPI_Bcast(buffer, count, datatype, root_rank, cur_comm);
        }
        else
            rc = PMPI_Bcast(buffer, count, datatype, root, comm);
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
            int flag = (MPI_SUCCESS==rc);
            MPIX_Comm_agree(cur_comm, &flag);
            if(!flag && rc == MPI_SUCCESS)
                rc = MPIX_ERR_PROC_FAILED;
            if(rc == MPI_SUCCESS)
                return rc;
            else
                replace_comm(&cur_comm);
        }
        else
            return rc;
    }
}

/*
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int rank, size;
    PMPI_Comm_size(comm, &size);
    PMPI_Comm_rank(comm, &rank);
    int rc = PMPI_Send(buf, count, datatype, dest, tag, comm);
    MPI_Error_string(rc, errstr, &len);
    printf("Rank %d / %d: send done (error: %s)\n", rank, size, errstr);
    return rc;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    int rank, size;
    PMPI_Comm_size(comm, &size);
    PMPI_Comm_rank(comm, &rank);
    int rc = PMPI_Recv(buf, count, datatype, source, tag, comm, status);
    MPI_Error_string(rc, errstr, &len);
    printf("Rank %d / %d: recv done (error: %s)\n", rank, size, errstr);
    return rc;
}


int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int rank, size;
    PMPI_Comm_size(comm, &size);
    PMPI_Comm_rank(comm, &rank);
    int rc = PMPI_Bcast(buffer, count, datatype, root, comm);
    MPI_Error_string(rc, errstr, &len);
    printf("Rank %d / %d: bcast done (error: %s)\n", rank, size, errstr);
    return rc;
}
*/