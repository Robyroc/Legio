#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "adv_comm.h"
#include "multicomm.h"
#include "operations.h"


int VERBOSE = 1;

char errstr[MPI_MAX_ERROR_STRING];
int len;
Multicomm *cur_comms;

int MPI_Init(int* argc, char *** argv)
{
    int rc = PMPI_Init(argc, argv);
    initialization();
    return rc;
    /*
    int provided;
    return MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
    */
}

int MPI_Init_thread(int* argc, char *** argv, int required, int* provided)
{
    int rc = PMPI_Init_thread(argc, argv, required, provided);
    initialization();
    //kalive_thread();
    return rc;
}

int MPI_Finalize()
{
    MPI_Barrier(MPI_COMM_WORLD);
    //kill_kalive_thread();
    finalization();
    //return PMPI_Finalize();
    return MPI_SUCCESS;
}

int MPI_Abort(MPI_Comm comm, int errorcode)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_adv(comm);

        LocalOnly func([errorcode] (MPI_Comm comm_t, AdvComm*) -> int {
            return PMPI_Abort(comm_t, errorcode);
        }, false);

        rc = translated->perform_operation(func);
        
        print_info("abort", comm, rc);

        if(!flag)
            delete translated;

        if(!flag || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);

    CommCreator func([newcomm] (MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc;
        rc = PMPI_Comm_dup(comm_t, newcomm);
        agree_and_eventually_replace(&rc, adv, comm_t);
        return rc;
    }, false);

    rc = translated->perform_operation(func);
    
    print_info("comm_dup", comm, rc);

    if(!flag)
        delete translated;

    if(!flag)
        return rc;
    else if(rc == MPI_SUCCESS)
    {
        MPI_Comm_set_errhandler(*newcomm, MPI_ERRORS_RETURN);
        bool result = add_comm(*newcomm, translated);
        if(result)
            return rc;
    }
}

int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        if(flag)
        {
            int rank;
            MPI_Group_rank(group, &rank);
            rank = (rank != MPI_UNDEFINED);
            MPI_Comm temp;
            rc = MPI_Comm_split(comm, rank, 0, &temp);
            if(rank)
                *newcomm = temp;
            else
            {
                MPI_Comm_disconnect(&temp);
                *newcomm = MPI_COMM_NULL;
            }
            return rc;
        }
        else
            return PMPI_Comm_create(comm, group, newcomm);
    }
}
/*
int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);

    CommCreator func([group, tag, newcomm, comm] (MPI_Comm comm_t) -> int {
        MPI_Group new_group, failed_group;
        MPIX_Comm_failure_get_acked(comm, &failed_group);
        MPI_Group_difference(group, failed_group, &new_group);
        return PMPI_Comm_create_group(comm_t, new_group, tag, newcomm);
    }, false);

    if(flag)
    {
        rc = translated->perform_operation(func);
    }
    else
        rc = func(comm);
    
    print_info("comm_create_group", comm, rc);

    if(flag)
    {
        agree_and_eventually_replace(&rc, translated);
        if(rc == MPI_SUCCESS)
        {
            MPI_Comm_set_errhandler(*newcomm, MPI_ERRORS_RETURN);
            bool result = add_comm(*newcomm);
            if(result)
                return rc;
        }
    }
    else
        return rc;
}
*/

int MPI_Comm_disconnect(MPI_Comm * comm)
{
    std::function<int(MPI_Comm*)> func = [](MPI_Comm * a){return PMPI_Comm_disconnect(a);};
    cur_comms->remove(*comm, func);
    func(comm);
    return MPI_SUCCESS;
}

int MPI_Comm_free(MPI_Comm* comm)
{
    std::function<int(MPI_Comm*)> func = [](MPI_Comm * a){return PMPI_Comm_free(a);};
    cur_comms->remove(*comm, func);
    func(comm);
    return MPI_SUCCESS;
}

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm* newcomm)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);

    CommCreator func ([color, key, newcomm] (MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc = PMPI_Comm_split(comm_t, color, key, newcomm);
        agree_and_eventually_replace(&rc, adv, comm_t);
        return rc;
    }, false);

    rc = translated->perform_operation(func);
    
    print_info("comm_split", comm, rc);

    if(!flag)
        delete translated;

    if(!flag)
        return rc;
    else if(rc == MPI_SUCCESS)
    {
        MPI_Comm_set_errhandler(*newcomm, MPI_ERRORS_RETURN);
        bool result = add_comm(*newcomm, translated);
        if(result)
            return rc;
    }
}

int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);

    AllToOne first([](int, MPI_Comm, AdvComm*) -> int {return MPI_SUCCESS;}, false);
    OneToAll second([info] (int, MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc = PMPI_Comm_set_info(comm_t, info);
        agree_and_eventually_replace(&rc, adv, comm_t);
        return rc;
    }, false);

    AllToAll func ([info] (MPI_Comm comm_t, AdvComm* adv) -> int {
        int rc = PMPI_Comm_set_info(comm_t, info);
        agree_and_eventually_replace(&rc, adv, comm_t);
        return rc;
    }, false, {first, second});

    rc = translated->perform_operation(func);
    
    print_info("comm_set_info", comm, rc);

    if(!flag)
        delete translated;

    return rc;
}


int MPI_Comm_get_info(MPI_Comm comm, MPI_Info * info_used)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    AdvComm* translated = cur_comms->translate_into_adv(comm);

    LocalOnly func ([info_used] (MPI_Comm comm_t, AdvComm* adv) -> int {
        return PMPI_Comm_get_info(comm_t, info_used);
    }, false);

    if(flag)
        rc = translated->perform_operation(func);
    
    print_info("comm_get_info", comm, rc);

    if(!flag)
        delete translated;

    return rc;
}