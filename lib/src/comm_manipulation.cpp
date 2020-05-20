#include "comm_manipulation.h"
#include <mpi.h>
#include <mpi-ext.h>
#include "complex_comm.h"
#include "multicomm.h"

extern Multicomm *cur_comms;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

void translate_ranks(int root, ComplexComm* comm, int* tr_rank)
{
    MPI_Group tr_group;
    int source = root;
    MPI_Comm_group(comm->get_comm(), &tr_group);
    MPI_Group_translate_ranks(comm->get_group(), 1, &source, tr_group, tr_rank);
}

void replace_comm(ComplexComm* cur_complex)
{
    MPI_Comm new_comm;
    int old_size, new_size, diff;
    MPIX_Comm_shrink(cur_complex->get_comm(), &new_comm);
    MPI_Comm_size(cur_complex->get_comm(), &old_size);
    MPI_Comm_size(new_comm, &new_size);
    diff = old_size - new_size; /* number of deads */
    if(0 == diff)
        MPI_Comm_free(&new_comm);
    else
    {
        MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
        cur_comms->change_comm(cur_complex, new_comm);
    }
}

void agree_and_eventually_replace(int* rc, ComplexComm* cur_complex)
{
    int flag = (MPI_SUCCESS==*rc);
    MPIX_Comm_agree(cur_complex->get_comm(), &flag);
    if(!flag && *rc == MPI_SUCCESS)
        *rc = MPIX_ERR_PROC_FAILED;
    if(*rc != MPI_SUCCESS)
        replace_comm(cur_complex);
}

int MPI_Barrier(ComplexComm* comm)
{
    while(1)
    {
        int rc;
        rc = PMPI_Barrier(comm->get_comm());
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm->get_comm(), &size);
            PMPI_Comm_rank(comm->get_comm(), &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: barrier done (error: %s)\n", rank, size, errstr);
        }
        
        if(rc == MPI_SUCCESS)
            return rc;
        else
            replace_comm(comm);
    }
}