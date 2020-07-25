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

int any_recv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status*);

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int i, rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);

    OneToOne func([buf, count, datatype, tag] (int other_t, MPI_Comm comm_t, AdvComm* adv) -> int {
        if(other_t == MPI_UNDEFINED)
        {
            HANDLE_SEND_FAIL(comm_t);
        }
        return PMPI_Send(buf, count, datatype, other_t, tag, comm_t);
    }, false);

    for(i = 0; i < NUM_RETRY; i++)
    {
        rc = translated->perform_operation(func, dest);
        
        print_info("send", comm, rc);

        if(!flag)
            delete translated;

        if(rc == MPI_SUCCESS)
            return rc;
    }
    return rc;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    if(source == MPI_ANY_SOURCE)
        return any_recv(buf, count, datatype, source, tag, comm, status);

    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);

    OneToOne func([buf, count, datatype, tag, status] (int other_t, MPI_Comm comm_t, AdvComm* adv) -> int {
        if(other_t == MPI_UNDEFINED)
        {
            HANDLE_RECV_FAIL(comm_t);
        }
        int rc = PMPI_Recv(buf, count, datatype, other_t, tag, comm_t, status);
        if(rc != MPI_SUCCESS)
        {
            HANDLE_RECV_FAIL(comm_t);
        }
        else
            return rc;
    }, false);

    rc = translated->perform_operation(func, source);
    
    print_info("recv", comm, rc);

    if(!flag)
        delete translated;
    return rc;
}

int any_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);
    
    OneToOne func([buf, count, datatype, tag, status] (int other_t, MPI_Comm comm_t, AdvComm*) -> int {
        int rc;
        rc = PMPI_Recv(buf, count, datatype, other_t, tag, comm_t, status);
        if(rc != MPI_SUCCESS)
            MPIX_Comm_failure_ack(comm_t);
    }, true);

    rc = translated->perform_operation(func, source);
    
    print_info("anyrecv", comm, rc);

    if(!flag)
        delete translated;
        
    return rc;
}