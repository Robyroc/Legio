#include "restart_routines.hpp"
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <vector>
#include "complex_comm.hpp"
#include "mpi.h"
#include "multicomm.hpp"

#include "mpi-ext.h"

extern "C" {
#include "legio.h"
}

using namespace legio;

// Failure mutex is locked in shared mode when accessing normal MPI operations
// When a restart operation is started, we need to lock it exclusively
std::shared_timed_mutex failure_mtx;
std::mutex change_world_mtx;

void legio::repair_failure()
{
    // Failure repair procedure needed - for all ranks
    int old_size, new_size, failed, ranks[LEGIO_MAX_FAILS], i, rank, original_world_size, flag;
    MPI_Comm_size(MPI_COMM_WORLD, &original_world_size);
    std::vector<int> failed_world_ranks;
    ComplexComm& world = Multicomm::get_instance().translate_into_complex(MPI_COMM_WORLD);

    // Ensure all failed ranks are acked
    // MPIX_Comm_revoke(world->get_comm());
    MPIX_Comm_failure_ack(world.get_comm());
    who_failed(world.get_comm(), &failed, ranks);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (failed == 0)
    {
        // We already repaired, let's exit!
        /*
        if (VERBOSE)
        {
            int rank, size;
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            printf("Skipping to repair failure from rank %d / %d\n", rank, size);
            fflush(stdout);
        }
        */
        return;
    }
    else
    {
        /*
        if (VERBOSE)
        {
            int rank, size;
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            printf("Found a number of process failed (%d) from rank %d / %d\n", failed, rank, size);
            fflush(stdout);
        }
        */
    }

    MPI_Comm tmp_intracomm, tmp_intercomm, tmp_world, new_world;
    // PMPI_Comm_size(world->get_comm(), &rank);
    PMPIX_Comm_shrink(world.get_comm(), &tmp_world);
    PMPI_Comm_size(tmp_world, &new_size);

    // Transform the failed processes to world alias ranks
    std::vector<Rank> world_ranks = Multicomm::get_instance().get_ranks();
    for (i = 0; i < failed; i++)
    {
        failed_world_ranks.push_back(Multicomm::get_instance().untranslate_world_rank(ranks[i]));
        // printf("FAILED RANK: %d TRANSLATED %d\n", ranks[i],
        // Multicomm::get_instance().untranslate_world_rank(ranks[i]));
        fflush(stdout);
    }

    // Set the ranks as failed and gather to respawn
    std::vector<int> current_to_respawn;
    for (auto failed_world_rank : failed_world_ranks)
    {
        if (std::find(Multicomm::get_instance().get_respawn_list().begin(),
                      Multicomm::get_instance().get_respawn_list().end(),
                      failed_world_rank) == Multicomm::get_instance().get_respawn_list().end())
        {
            Multicomm::get_instance().set_failed_rank(failed_world_rank);
        }
        else
        {
            current_to_respawn.push_back(failed_world_rank);
            /*
            printf("I should respawn the following rank: %d\n", failed_world_rank);
            fflush(stdout);
            */
        }
    }

    std::vector<int> all_failed_ranks;
    for (auto cur_rank : Multicomm::get_instance().get_ranks())
    {
        if (cur_rank.failed)
        {
            all_failed_ranks.push_back(cur_rank.number);
        }
    }

    // Re-generate all MPI_Comm_world for all the restarted processes
    std::vector<char*> program_names;
    std::vector<char**> argvs;
    std::vector<int> max_procs;
    std::vector<MPI_Info> infos;
    for (auto to_respawn : current_to_respawn)
    {
        char** newargv = (char**)malloc(sizeof(char*) * 10);
        for (i = 0; i < 10; i++)
        {
            newargv[i] = (char*)malloc(sizeof(char) * 30);
        }
        sprintf(newargv[0], "--respawned");
        sprintf(newargv[1], "--rank");
        sprintf(newargv[2], "%d", to_respawn);
        sprintf(newargv[3], "--size");
        sprintf(newargv[4], "%d", original_world_size);
        sprintf(newargv[5], "--to-respawn");
        std::string separator;
        std::ostringstream ss;
        for (auto x : Multicomm::get_instance().get_respawn_list())
        {
            ss << separator << x;
            separator = ",";
        }
        sprintf(newargv[6], "%s", ss.str().c_str());

        if (Multicomm::get_instance().get_ranks().size() != 0)
        {
            newargv[7] = NULL;
            newargv[8] = NULL;
        }
        else
        {
            sprintf(newargv[7], "--failed-ranks");
            std::string separator_failed_ranks;
            std::ostringstream ss_failed_ranks;
            for (auto x : all_failed_ranks)
            {
                ss_failed_ranks << separator_failed_ranks << x;
                separator_failed_ranks = ",";
            }
            sprintf(newargv[8], "%s", ss_failed_ranks.str().c_str());
        }
        newargv[9] = NULL;
        argvs.push_back(newargv);
        program_names.push_back(program_invocation_name);
        max_procs.push_back(1);
        infos.push_back(MPI_INFO_NULL);
    }

    if (current_to_respawn.size() != 0)
    {
        PMPI_Comm_spawn_multiple(current_to_respawn.size(), program_names.data(), argvs.data(),
                                 max_procs.data(), infos.data(), 0, tmp_world, &tmp_intercomm,
                                 NULL);
        PMPI_Intercomm_merge(tmp_intercomm, 1, &tmp_intracomm);
        PMPI_Comm_split(tmp_intracomm, 1, rank, &new_world);
        /*
        printf("Completed respawn of process from rank %d\n", rank);
        fflush(stdout);
        */
    }
    else
    {
        /*
        printf("NO RANK TO RESPAWN from process %d!\n", rank);
        fflush(stdout);
        */
        new_world = tmp_world;
    }
    MPI_Comm_set_errhandler(new_world, MPI_ERRORS_RETURN);
    Multicomm::get_instance().translate_into_complex(MPI_COMM_WORLD).replace_comm(new_world);

    MPI_Group group_world;
    PMPI_Comm_group(new_world, &group_world);
    ComplexComm& complex = Multicomm::get_instance().translate_into_complex(MPI_COMM_WORLD);

    // Regenerate the supported comms
    for (auto entry : Multicomm::get_instance().supported_comms_vector)
    {
        /*
        printf("Rank %d is regenerating the supported comm\n", rank);
        */
        auto world_ranks = entry.get_current_world_ranks();
        // Get the group of current world ranks
        std::vector<int> ranks_in_comm, ranks_in_comm_translated;
        MPI_Group new_group;
        MPI_Comm new_comm, alias_comm = entry.alias;
        for (auto rank : world_ranks)
        {
            if (!rank.failed)
                ranks_in_comm.push_back(rank.number);
        }

        // Translate the ranks according to current world
        for (auto rank : ranks_in_comm)
        {
            int dest = Multicomm::get_instance().translate_ranks(rank, complex);
            ranks_in_comm_translated.push_back(dest);
        }

        MPI_Group_incl(group_world, ranks_in_comm_translated.size(),
                       ranks_in_comm_translated.data(), &new_group);

        PMPI_Comm_create(new_world, new_group, &new_comm);
        Multicomm::get_instance().translate_into_complex(alias_comm).replace_comm(new_comm);
        MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
    }

    /*
    if (VERBOSE)
    {
        int rank, size;
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        printf("Rank %d / %d: completed respawning process for current failure.\n", rank, size);
        fflush(stdout);
    }
    */
}

void legio::loop_repair_failures()
{
    int rank;

    while (1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        int length;
        MPI_Status status;
        MPI_Comm world_comm;
        change_world_mtx.lock();
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        ComplexComm& world = Multicomm::get_instance().translate_into_complex(MPI_COMM_WORLD);
        world_comm = world.get_comm();
        MPI_Comm_set_errhandler(world_comm, MPI_ERRORS_RETURN);
        if (world_comm == MPI_COMM_NULL)
        {
            change_world_mtx.unlock();
            return;
        }
        // printf("Probing from rank %d\n", rank);
        int flag = 0, flag_self = 0, buf;
        int local_rank;
        PMPI_Iprobe(MPI_ANY_SOURCE, LEGIO_FAILURE_TAG, world_comm, &flag, MPI_STATUS_IGNORE);
        if (flag)
        {
            // failure_mtx.lock();
            int rank, size;
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            printf("\n\n\n%d FOUND SOMETHING TO REPAIR THROUGH THREAD!\n", rank);
            PMPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, LEGIO_FAILURE_TAG, world_comm, &status);
            /*
            if (VERBOSE)
            {
                printf("Rank %d / %d: received failure notification from %d.\n", rank, size,
                       status.MPI_SOURCE);
                fflush(stdout);
            }
            */
            change_world_mtx.unlock();
            failure_mtx.lock();
            repair_failure();
            failure_mtx.unlock();
        }
        else
            change_world_mtx.unlock();
    }
}

void legio::restart(int rank)
{
    // Restart and re-construct MPI_COMM_WORLD
    MPI_Comm parent, intra_tmp, new_world;
    // Merge getting back the entire MPI_COMM_WORLD
    PMPI_Comm_get_parent(&parent);
    PMPI_Intercomm_merge(parent, 1, &intra_tmp);

    // Split to re-assign ranks
    PMPI_Comm_split(intra_tmp, 1, rank, &new_world);

    MPI_Comm_set_errhandler(new_world, MPI_ERRORS_RETURN);
    // Reassign world with the merged comm
    Multicomm::get_instance().translate_into_complex(MPI_COMM_WORLD).replace_comm(new_world);

    /*
    if (VERBOSE)
    {
        printf("Rank %d respawn complete.\n", rank);
        fflush(stdout);
    }
    */
}