#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "complex_comm.h"
#include "multicomm.h"
#include "intercomm_utils.h"


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
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    ComplexComm* translated = cur_comms->translate_into_complex(comm);
    if(flag)
        rc = PMPI_Abort(translated->get_comm(), errorcode);
    else
        rc = PMPI_Abort(comm, errorcode);
    if(VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: abort done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        ComplexComm* translated = cur_comms->translate_into_complex(comm);
        if(flag)
            rc = PMPI_Comm_dup(translated->get_comm(), newcomm);
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
        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if(rc == MPI_SUCCESS)
            {
                MPI_Comm_set_errhandler(*newcomm, MPI_ERRORS_RETURN);
                bool result = cur_comms->add_comm(
                    *newcomm,
                    comm,
                    [](MPI_Comm source, MPI_Comm* dest) -> int
                    {
                        int rc = PMPI_Comm_dup(source, dest);
                        MPI_Comm_set_errhandler(*dest, MPI_ERRORS_RETURN);
                        return rc;
                    });
                if(result)
                    return rc;
            }
        }
        else
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

int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    ComplexComm* translated = cur_comms->translate_into_complex(comm);
    if(flag)
    {
        MPI_Group first_clean, second_clean;
        check_group(*translated, group, &first_clean, &second_clean);
        int size_first, size_second;
        MPI_Group_size(first_clean, &size_first);
        MPI_Group_size(second_clean, &size_second);
        if (size_first != size_second)
        {
            rc = MPI_ERR_PROC_FAILED;
            *newcomm = MPI_COMM_NULL;
        }
        else
            rc = PMPI_Comm_create_group(translated->get_comm(), second_clean, tag, newcomm);
    }
    else
        rc = PMPI_Comm_create_group(comm, group, tag, newcomm);
    if(VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: comm_create_group done (error: %s)\n", rank, size, errstr);
    }
    if(flag && rc == MPI_SUCCESS)
    {
        MPI_Comm_set_errhandler(*newcomm, MPI_ERRORS_RETURN);
        cur_comms->add_comm(
            *newcomm,
            comm,
            [group, tag] (MPI_Comm source, MPI_Comm *dest) -> int 
            {
                //TODO update me
                int rc = PMPI_Comm_create_group(source, group, tag, dest);
                MPI_Comm_set_errhandler(*dest, MPI_ERRORS_RETURN);
                return rc;
            });
        return rc;
    }
    else
        return rc;
}

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
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        ComplexComm* translated = cur_comms->translate_into_complex(comm);
        if(flag)
            rc = PMPI_Comm_split(translated->get_comm(), color, key, newcomm);
        else
            rc = PMPI_Comm_split(comm, color, key, newcomm);
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: comm_split done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if(rc == MPI_SUCCESS)
            {
                MPI_Comm_set_errhandler(*newcomm, MPI_ERRORS_RETURN);
                bool result = cur_comms->add_comm(
                    *newcomm,
                    comm,
                    [color, key] (MPI_Comm source, MPI_Comm *dest) -> int
                    {
                        int rc = PMPI_Comm_split(source, color, key, dest);
                        MPI_Comm_set_errhandler(*dest, MPI_ERRORS_RETURN);
                        return rc;
                    });
                if(result)
                    return rc;
            }
        }
        else
            return rc;
    }
}

int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm)
{
    while(1)
    {
        int rc, flag, own_rank;
        cur_comms->part_of(local_comm, &flag);
        PMPI_Comm_rank(local_comm, &own_rank);
        //MPI_Barrier(local_comm);
        MPI_Comm remote_comm = MPI_COMM_NULL;
        ComplexComm* translated = cur_comms->translate_into_complex(local_comm);
        if(flag)
        {
            int local_root;
            translate_ranks(local_leader, translated, &local_root);
            if(own_rank == local_leader)
            {
                MPI_Group remote_group, shrink_group;
                MPI_Comm_group(peer_comm, &remote_group);
                int remote_rank;
                MPI_Comm_rank(peer_comm, &remote_rank);
                int indexes[2] = {(remote_rank < remote_leader ? remote_rank : remote_leader), (remote_rank < remote_leader ? remote_leader : remote_rank)};
                MPI_Group_incl(remote_group, 2, indexes, &shrink_group);
                rc = MPI_Comm_create_group(peer_comm, shrink_group, 0, &remote_comm);
                //TODO handle faults of root nodes
                MPI_Barrier(local_comm);
                rc = PMPI_Intercomm_create(translated->get_comm(), local_root, remote_comm, (remote_rank < remote_leader ? 1 : 0), tag, newintercomm);
                MPI_Comm_free(&remote_comm);
            }
            else
            {
                MPI_Barrier(local_comm);
                rc = PMPI_Intercomm_create(translated->get_comm(), local_root, remote_comm, 0, tag, newintercomm);
            }
        }
        else
            rc = PMPI_Intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(local_comm, &size);
            PMPI_Comm_rank(local_comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: intercomm_create done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if(rc == MPI_SUCCESS)
            {
                MPI_Group local_group, remote_group;
                MPI_Comm_group(local_comm, &local_group);
                MPI_Comm_group(peer_comm, &remote_group);
                MPI_Comm_set_errhandler(*newintercomm, MPI_ERRORS_RETURN);
                int result = cur_comms->add_comm(
                    *newintercomm,
                    local_comm,
                    nullptr,
                    [local_leader, remote_leader, tag, local_group, remote_group] (MPI_Comm source1, MPI_Comm source2, MPI_Comm *dest) -> int
                    {
                        MPI_Group source1_group, source2_group;
                        MPI_Comm_group(source1, &source1_group);
                        MPI_Comm_group(source2, &source2_group);
                        int rank_1, rank_2;
                        MPI_Group_translate_ranks(local_group, 1, &local_leader, source1_group, &rank_1);
                        MPI_Group_translate_ranks(remote_group, 1, &remote_leader, source2_group, &rank_2);
                        int rc = PMPI_Intercomm_create(source1, rank_1, source2, rank_2, tag, dest);
                        MPI_Comm_set_errhandler(*dest, MPI_ERRORS_RETURN);
                        return rc;
                    }, peer_comm);
                if(result)
                    return rc;
            }
        }
        else
            return rc;
    }
}

int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(intercomm, &flag);
        ComplexComm* translated = cur_comms->translate_into_complex(intercomm);
        if(flag)
            rc = PMPI_Intercomm_merge(translated->get_comm(), high, newintracomm);
        else
            rc = PMPI_Intercomm_merge(intercomm, high, newintracomm);
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(intercomm, &size);
            PMPI_Comm_rank(intercomm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: intercomm_merge done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if(rc == MPI_SUCCESS)
            {
                MPI_Comm_set_errhandler(*newintracomm, MPI_ERRORS_RETURN);
                bool result = cur_comms->add_comm(
                    *newintracomm,
                    intercomm,
                    [high](MPI_Comm source, MPI_Comm* dest) -> int
                    {
                        int rc = PMPI_Intercomm_merge(source, high, dest);
                        MPI_Comm_set_errhandler(*dest, MPI_ERRORS_RETURN);
                        return rc;
                    });
                if(result)
                    return rc;
            }
        }
        else
            return rc;
    }
}

int MPI_Comm_spawn(const char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        ComplexComm* translated = cur_comms->translate_into_complex(comm);
        int root_rank = root;
        translate_ranks(root, translated, &root_rank);
        if(flag)
            rc = PMPI_Comm_spawn(command, argv, maxprocs, info, root_rank, translated->get_comm(), intercomm, array_of_errcodes);
        else
            rc = PMPI_Comm_spawn(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes);
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: comm_spawn done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if(rc == MPI_SUCCESS)
            {
                MPI_Comm_set_errhandler(*intercomm, MPI_ERRORS_RETURN);
                bool result = cur_comms->add_comm(
                    *intercomm,
                    comm,
                    [root_rank, command, argv, maxprocs, info] (MPI_Comm source, MPI_Comm *dest) -> int
                    {
                        int* array_of_errcodes;
                        int rc = PMPI_Comm_spawn(command, argv, maxprocs, info, root_rank, source, dest, array_of_errcodes);
                        MPI_Comm_set_errhandler(*dest, MPI_ERRORS_RETURN);
                        return rc;
                    });
                if(result)
                    return rc;
            }
        }
    }
}

int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        ComplexComm* translated = cur_comms->translate_into_complex(comm);
        if(flag)
            rc = PMPI_Comm_set_info(translated->get_comm(), info);
        else
            rc = PMPI_Comm_set_info(comm, info);
        if(VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: comm_set_info done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}


int MPI_Comm_get_info(MPI_Comm comm, MPI_Info * info_used)
{
    int rc, flag;
    cur_comms->part_of(comm, &flag);
    ComplexComm* translated = cur_comms->translate_into_complex(comm);
    if(flag)
        rc = PMPI_Comm_get_info(translated->get_comm(), info_used);
    else
        rc = PMPI_Comm_get_info(comm, info_used);
    if(VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: comm_get_info done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}