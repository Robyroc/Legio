#include "comm_manipulation.h"
#include <mpi.h>
#include <mpi-ext.h>
#include "adv_comm.h"
#include "single_comm.h"
#include "no_comm.h"
#include "hierar_comm.h"
#include "multicomm.h"
#include <string>
#include "operations.h"
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
    AdvComm* bogus_world = new NoComm(MPI_COMM_WORLD);
    add_comm(MPI_COMM_WORLD, bogus_world);
    MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
    AdvComm* bogus_self = new NoComm(MPI_COMM_SELF);
    add_comm(MPI_COMM_SELF, bogus_self);
    delete bogus_world; delete bogus_self;
}

bool add_comm(MPI_Comm comm, AdvComm* source)
{
    return source->add_comm(comm);
}

bool add_comm(MPI_Comm comm, NoComm* source)
{
    return cur_comms->add_comm<SingleComm>(comm);
}

bool add_comm(MPI_Comm comm, SingleComm* source)
{
    return cur_comms->add_comm<SingleComm>(comm);
}

bool add_comm(MPI_Comm comm, HierarComm* source)
{
    return cur_comms->add_comm<HierarComm>(comm);
}

void finalization()
{
    delete cur_comms;
}

void replace_comm(AdvComm* cur_complex)
{
    cur_complex->fault_manage();
}

void agree_and_eventually_replace(int* rc, AdvComm* cur_complex)
{
    int flag = (MPI_SUCCESS==*rc);
    int* pointer = &flag;
    
    cur_complex->result_agreement(pointer);

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