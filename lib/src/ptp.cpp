#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "adv_comm.h"
#include "multicomm.h"

extern Multicomm *cur_comms;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

int any_recv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status*);

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int i, rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_complex(comm);

    OneToOne func([buf, count, datatype, tag] (int other_t, MPI_Comm comm_t) -> int {
        if(other_t == MPI_UNDEFINED)
        {
            HANDLE_SEND_FAIL(comm_t);
        }
        return PMPI_Send(buf, count, datatype, other_t, tag, comm_t);
    }, false);

    for(i = 0; i < NUM_RETRY; i++)
    {
        if(flag)
        {
            rc = translated->perform_operation(func, dest);
        }
        else
            rc = func(dest, comm);
        
        print_info("send", comm, rc);

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
    AdvComm* translated = cur_comms->translate_into_complex(comm);

    OneToOne func([buf, count, datatype, tag, status] (int other_t, MPI_Comm comm_t) -> int {
        if(other_t == MPI_UNDEFINED)
        {
            HANDLE_RECV_FAIL(comm_t);
        }
        return PMPI_Recv(buf, count, datatype, other_t, tag, comm_t, status);
    }, false);

    if(flag)
    {
        rc = translated->perform_operation(func, source);
    }
    else
        rc = func(source, comm);
    
    print_info("recv", comm, rc);

    return rc;
}

int any_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_complex(comm);
    
    OneToOne func([buf, count, datatype, tag, status] (int other_t, MPI_Comm comm_t) -> int {
        return PMPI_Recv(buf, count, datatype, other_t, tag, comm_t, status);
    }, true);

    if(flag)
    {
        rc = translated->perform_operation(func, source);
    }
    else
        rc = func(source, comm);
    
    print_info("anyrecv", comm, rc);

    if(rc != MPI_SUCCESS)
    {
        /*
        int eclass;
        MPI_Error_class(rc, &eclass);
        if( MPIX_ERR_PROC_FAILED != eclass ) 
        {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        */
        //MPIX_Comm_failure_ack(translated->get_comm());    FIX ME
    }
    return rc;
}