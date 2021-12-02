#include "comm_manipulation.h"
#include <mpi.h>
#include <mpi-ext.h>
#include "complex_comm.h"
#include "multicomm.h"
//#include <thread>

extern Multicomm *cur_comms;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;
//std::thread * kalive;
//int temp;

void initialization()
{
    cur_comms = new Multicomm();
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    cur_comms->add_comm(
        MPI_COMM_WORLD,
        MPI_COMM_NULL,
        [](MPI_Comm a, MPI_Comm* dest) -> int 
        {
            *dest = MPI_COMM_WORLD;
            MPI_Comm_set_errhandler(*dest, MPI_ERRORS_RETURN);
            return MPI_SUCCESS;
        });
    MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
    cur_comms->add_comm(
        MPI_COMM_SELF,
        MPI_COMM_NULL,
        [](MPI_Comm a, MPI_Comm* dest) -> int
        {
            *dest = MPI_COMM_SELF;
            MPI_Comm_set_errhandler(*dest, MPI_ERRORS_RETURN);
            return MPI_SUCCESS;
        });
}

void finalization()
{
    delete cur_comms;
}

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
        PMPI_Comm_free(&new_comm);
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
/*
void receiver()
{
    MPI_Recv(&temp, 1, MPI_INT, 0, 75, MPI_COMM_SELF, MPI_STATUS_IGNORE);
}

void sender()
{
    int i = 0;
    MPI_Send(&i, 1, MPI_INT, 0, 75, MPI_COMM_SELF);
}

void kalive_thread()
{
    kalive = new std::thread(receiver);
}

void kill_kalive_thread()
{
    sender();
}
*/