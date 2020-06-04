#include "comm_manipulation.h"
#include <mpi.h>
#include <mpi-ext.h>
#include "adv_comm.h"
#include "single_comm.h"
#include "multicomm.h"
#include <string>
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
    add_comm(MPI_COMM_WORLD);
    MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
    add_comm(MPI_COMM_SELF);
}

bool add_comm(MPI_Comm comm)
{
    cur_comms->add_comm<SingleComm>(comm);
}

void finalization()
{
    delete cur_comms;
}

void translate_ranks(int root, AdvComm* comm, int* tr_rank)
{
    MPI_Group tr_group;
    int source = root;
    MPI_Comm_group(comm->get_comm(), &tr_group);
    MPI_Group_translate_ranks(comm->get_group(), 1, &source, tr_group, tr_rank);
}

void replace_comm(AdvComm* cur_complex)
{
    cur_complex->fault_manage();
}

void agree_and_eventually_replace(int* rc, AdvComm* cur_complex)
{
    int flag = (MPI_SUCCESS==*rc);
    MPIX_Comm_agree(cur_complex->get_comm(), &flag);
    if(!flag && *rc == MPI_SUCCESS)
        *rc = MPIX_ERR_PROC_FAILED;
    if(*rc != MPI_SUCCESS)
        replace_comm(cur_complex);
}

void print_info(std::string method, MPI_Comm comm, int rc)
{
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: %s done (error: %s)\n", rank, size, method.c_str(), errstr);
    }
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