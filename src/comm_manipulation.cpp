#include "comm_manipulation.h"
#include <mpi.h>
#include <mpi-ext.h>
#include "complex_comm.h"

MPI_Group world;
int initialized = 0;

void translate_ranks(int root, MPI_Comm comm, int* tr_rank)
{
    MPI_Group cur_group;
    int source = root;
    if(!initialized)
        MPI_Comm_group(MPI_COMM_WORLD, &world);

    MPI_Comm_group(comm, &cur_group);
    MPI_Group_translate_ranks(world, 1, &source, cur_group, tr_rank);
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
        cur_complex->replace_comm(new_comm);
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