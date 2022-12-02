#include <mpi.h>
#include <signal.h>
#include <stdio.h>
#include <shared_mutex>
#include <thread>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "configuration.hpp"
#include "intercomm_utils.hpp"
#include "mpi-ext.h"
#include "multicomm.hpp"
// #include "respawn_multicomm.hpp"
extern "C" {
#include "restart.h"
}

int VERBOSE = 1;

char errstr[MPI_MAX_ERROR_STRING];
int len;

extern std::shared_timed_mutex failure_mtx;

int MPI_Init(int* argc, char*** argv)
{
    /*
    int rc = PMPI_Init(argc, argv);
    initialization();
    return rc;
    */
    int provided;

    int rc = PMPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
    initialization(argc, argv);

    std::thread repair(loop_repair_failures);
    repair.detach();

    // kalive_thread();
    return rc;
}

int MPI_Init_thread(int* argc, char*** argv, int required, int* provided)
{
    int rc = PMPI_Init_thread(argc, argv, required, provided);
    initialization(argc, argv);

    printf("Starting failure repairing thread...\n");

    // std::thread repair(loop_repair_failures);

    // kalive_thread();
    return rc;
}

int MPI_Finalize()
{
    MPI_Barrier(MPI_COMM_WORLD);
    // kill_kalive_thread();
    finalization();
    // return PMPI_Finalize();
    return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm, int* rank)
{
    // If not respawned, use alias to get the rank
    if (!Multicomm::get_instance().is_respawned())
        return PMPI_Comm_rank(comm, rank);
    else
    {
        // RespawnMulticomm* respawned_comms = dynamic_cast<RespawnMulticomm*>(cur_comms);
        auto supported_comms = Multicomm::get_instance().access_supported_comms_respawned();
        auto found_comm = supported_comms.find(c2f<MPI_Comm>(comm));

        if (comm == MPI_COMM_WORLD)
            *rank = Multicomm::get_instance().get_own_rank();
        else if (found_comm == supported_comms.end())
            return PMPI_Comm_rank(comm, rank);
        else
            *rank = found_comm->second.rank();
        return MPI_SUCCESS;
    }
}

int MPI_Comm_size(MPI_Comm comm, int* size)
{
    // If not respawned, use alias to get the size
    if (!Multicomm::get_instance().is_respawned())
        return PMPI_Comm_size(comm, size);
    else
    {
        // RespawnMulticomm* respawned_comms = dynamic_cast<RespawnMulticomm*>(cur_comms);
        auto supported_comms = Multicomm::get_instance().access_supported_comms_respawned();
        auto found_comm = supported_comms.find(MPI_Comm_c2f(comm));

        if (comm == MPI_COMM_WORLD)
            *size = Multicomm::get_instance().get_ranks().size();
        else if (found_comm == supported_comms.end())
            return PMPI_Comm_size(comm, size);
        else
            *size = found_comm->second.size();
        return MPI_SUCCESS;
    }
}

int MPI_Abort(MPI_Comm comm, int errorcode)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(comm);
    if (flag)
    {
        ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
        rc = PMPI_Abort(translated.get_comm(), errorcode);
    }
    else
        rc = PMPI_Abort(comm, errorcode);
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: abort done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm* newcomm)
{
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(comm);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
            rc = PMPI_Comm_dup(translated.get_comm(), newcomm);
        }
        else
            rc = PMPI_Comm_dup(comm, newcomm);
        failure_mtx.unlock_shared();
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: comm_dup done (error: %s)\n", rank, size, errstr);
        }
        if (flag)
        {
            agree_and_eventually_replace(&rc,
                                         Multicomm::get_instance().translate_into_complex(comm));
            if (rc == MPI_SUCCESS)
            {
                MPI_Comm_set_errhandler(*newcomm, MPI_ERRORS_RETURN);
                bool result = Multicomm::get_instance().add_comm(*newcomm);
                if (result)
                    return rc;
            }
        }
        else
            return rc;
    }
}

int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm* newcomm)
{
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(comm);
        failure_mtx.lock_shared();
        if (flag)
        {
            int rank;
            MPI_Group_rank(group, &rank);
            rank = (rank != MPI_UNDEFINED);
            MPI_Comm temp;
            rc = MPI_Comm_split(comm, rank, 0, &temp);
            if (rank)
                *newcomm = temp;
            else
            {
                MPI_Comm_free(&temp);
                *newcomm = MPI_COMM_NULL;
            }
            failure_mtx.unlock_shared();
            return rc;
        }
        else
        {
            rc = PMPI_Comm_create(comm, group, newcomm);
            failure_mtx.unlock_shared();
            return rc;
        }
    }
}

int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm* newcomm)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(comm);
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
        MPI_Group first_clean, second_clean;
        check_group(translated, group, &first_clean, &second_clean);
        int size_first, size_second;
        MPI_Group_size(first_clean, &size_first);
        MPI_Group_size(second_clean, &size_second);
        if (size_first != size_second)
        {
            printf("\n\n FAILED!!!!\n\n");
            rc = MPI_ERR_PROC_FAILED;
            *newcomm = MPI_COMM_NULL;
        }
        else
            rc = PMPI_Comm_create_group(translated.get_comm(), second_clean, tag, newcomm);
    }
    else
        rc = PMPI_Comm_create_group(comm, group, tag, newcomm);
    failure_mtx.unlock_shared();
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: comm_create_group done (error: %s)\n", rank, size, errstr);
        fflush(stdout);
    }
    if (flag && rc == MPI_SUCCESS && *newcomm != MPI_COMM_NULL)
    {
        MPI_Comm_set_errhandler(*newcomm, MPI_ERRORS_RETURN);
        Multicomm::get_instance().add_comm(*newcomm);
        return rc;
    }
    else
        return rc;
}

int MPI_Comm_disconnect(MPI_Comm* comm)
{
    std::function<int(MPI_Comm*)> func = [](MPI_Comm* a) { return PMPI_Comm_disconnect(a); };
    Multicomm::get_instance().remove(*comm, func);
    func(comm);
    return MPI_SUCCESS;
}

int MPI_Comm_free(MPI_Comm* comm)
{
    std::function<int(MPI_Comm*)> func = [](MPI_Comm* a) { return PMPI_Comm_free(a); };
    Multicomm::get_instance().remove(*comm, func);
    func(comm);
    return MPI_SUCCESS;
}

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm* newcomm)
{
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(comm);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
            rc = PMPI_Comm_split(translated.get_comm(), color, key, newcomm);
        }
        else
            rc = PMPI_Comm_split(comm, color, key, newcomm);
        failure_mtx.unlock_shared();
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: comm_split done (error: %s)\n", rank, size, errstr);
        }
        if (flag)
        {
            agree_and_eventually_replace(&rc,
                                         Multicomm::get_instance().translate_into_complex(comm));
            if (rc == MPI_SUCCESS)
            {
                MPI_Comm_set_errhandler(*newcomm, MPI_ERRORS_RETURN);
                bool result = Multicomm::get_instance().add_comm(*newcomm);
                if (result)
                    return rc;
            }
        }
        else
            return rc;
    }
}

int MPI_Intercomm_create(MPI_Comm local_comm,
                         int local_leader,
                         MPI_Comm peer_comm,
                         int remote_leader,
                         int tag,
                         MPI_Comm* newintercomm)
{
    while (1)
    {
        int rc, own_rank;
        bool flag = Multicomm::get_instance().part_of(local_comm);
        PMPI_Comm_rank(local_comm, &own_rank);
        // MPI_Barrier(local_comm);
        MPI_Comm remote_comm = MPI_COMM_NULL;
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(local_comm);
            int local_root = Multicomm::get_instance().translate_ranks(local_leader, translated);
            if (own_rank == local_leader)
            {
                MPI_Group remote_group, shrink_group;
                MPI_Comm_group(peer_comm, &remote_group);
                int remote_rank;
                MPI_Comm_rank(peer_comm, &remote_rank);
                int indexes[2] = {(remote_rank < remote_leader ? remote_rank : remote_leader),
                                  (remote_rank < remote_leader ? remote_leader : remote_rank)};
                MPI_Group_incl(remote_group, 2, indexes, &shrink_group);
                rc = MPI_Comm_create_group(peer_comm, shrink_group, 0, &remote_comm);
                // TODO handle faults of root nodes
                MPI_Barrier(local_comm);
                rc =
                    PMPI_Intercomm_create(translated.get_comm(), local_root, remote_comm,
                                          (remote_rank < remote_leader ? 1 : 0), tag, newintercomm);
                MPI_Comm_free(&remote_comm);
            }
            else
            {
                MPI_Barrier(local_comm);
                rc = PMPI_Intercomm_create(translated.get_comm(), local_root, remote_comm, 0, tag,
                                           newintercomm);
            }
        }
        else
            rc = PMPI_Intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag,
                                       newintercomm);
        failure_mtx.unlock_shared();
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(local_comm, &size);
            PMPI_Comm_rank(local_comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: intercomm_create done (error: %s)\n", rank, size, errstr);
        }
        if (flag)
        {
            agree_and_eventually_replace(
                &rc, Multicomm::get_instance().translate_into_complex(local_comm));
            if (rc == MPI_SUCCESS)
            {
                MPI_Group local_group, remote_group;
                MPI_Comm_group(local_comm, &local_group);
                MPI_Comm_group(peer_comm, &remote_group);
                MPI_Comm_set_errhandler(*newintercomm, MPI_ERRORS_RETURN);
                int result = Multicomm::get_instance().add_comm(*newintercomm);
                if (result)
                    return rc;
            }
        }
        else
            return rc;
    }
}

int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm* newintracomm)
{
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(intercomm);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(intercomm);
            rc = PMPI_Intercomm_merge(translated.get_comm(), high, newintracomm);
        }
        else
            rc = PMPI_Intercomm_merge(intercomm, high, newintracomm);
        failure_mtx.unlock_shared();
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(intercomm, &size);
            PMPI_Comm_rank(intercomm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: intercomm_merge done (error: %s)\n", rank, size, errstr);
        }
        if (flag)
        {
            agree_and_eventually_replace(
                &rc, Multicomm::get_instance().translate_into_complex(intercomm));
            if (rc == MPI_SUCCESS)
            {
                MPI_Comm_set_errhandler(*newintracomm, MPI_ERRORS_RETURN);
                bool result = Multicomm::get_instance().add_comm(*newintracomm);
                if (result)
                    return rc;
            }
        }
        else
            return rc;
    }
}

int MPI_Comm_spawn(const char* command,
                   char* argv[],
                   int maxprocs,
                   MPI_Info info,
                   int root,
                   MPI_Comm comm,
                   MPI_Comm* intercomm,
                   int array_of_errcodes[])
{
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(comm);
        int root_rank = root;
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
            root_rank = Multicomm::get_instance().translate_ranks(root, translated);
            rc = PMPI_Comm_spawn(command, argv, maxprocs, info, root_rank, translated.get_comm(),
                                 intercomm, array_of_errcodes);
        }
        else
            rc = PMPI_Comm_spawn(command, argv, maxprocs, info, root, comm, intercomm,
                                 array_of_errcodes);
        failure_mtx.unlock_shared();
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: comm_spawn done (error: %s)\n", rank, size, errstr);
        }
        if (flag)
        {
            agree_and_eventually_replace(&rc,
                                         Multicomm::get_instance().translate_into_complex(comm));
            if (rc == MPI_SUCCESS)
            {
                MPI_Comm_set_errhandler(*intercomm, MPI_ERRORS_RETURN);
                bool result = Multicomm::get_instance().add_comm(*intercomm);
                if (result)
                    return rc;
            }
        }
    }
}

int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
{
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(comm);
        ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
        failure_mtx.lock_shared();
        if (flag)
            rc = PMPI_Comm_set_info(translated.get_comm(), info);
        else
            rc = PMPI_Comm_set_info(comm, info);
        failure_mtx.unlock_shared();
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: comm_set_info done (error: %s)\n", rank, size, errstr);
        }
        if (flag)
        {
            agree_and_eventually_replace(&rc, translated);
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_Comm_get_info(MPI_Comm comm, MPI_Info* info_used)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(comm);
    ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
    failure_mtx.lock_shared();
    if (flag)
        rc = PMPI_Comm_get_info(translated.get_comm(), info_used);
    else
        rc = PMPI_Comm_get_info(comm, info_used);
    failure_mtx.unlock_shared();
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(comm, &size);
        PMPI_Comm_rank(comm, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: comm_get_info done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}