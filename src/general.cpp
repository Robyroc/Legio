#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "complex_comm.h"
#include "multicomm.h"


int VERBOSE = 0;

char errstr[MPI_MAX_ERROR_STRING];
int len;
Multicomm *cur_comms;

int MPI_Init(int* argc, char *** argv)
{
    int rc = PMPI_Init(argc, argv);
    cur_comms = new Multicomm();
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    cur_comms->add_comm(MPI_COMM_WORLD);
    return rc;
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    ComplexComm* translated = cur_comms->translate_into_complex(comm);
    if(flag)
    {
        rc = PMPI_Comm_dup(translated->get_comm(), newcomm);
        if(rc == MPI_SUCCESS)
        {
            MPI_Comm_set_errhandler(*newcomm, MPI_ERRORS_RETURN);
            cur_comms->add_comm(*newcomm);
        }
    }
    else
        rc = PMPI_Comm_dup(comm, newcomm);
    if(VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: comm_dup done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}