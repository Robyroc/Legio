#include "mpi.h"
#include "mpi-ext.h"
#include "comm_manipulation.h"
#include "complex_comm.h"
#include "supported_comm.h"
#include "respawn_multicomm.h"
#include "multicomm.h"
#include "legio.h"
#include <functional>
#include <chrono>
#include <string>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <sstream>
#include <unistd.h>
//#include <thread>

extern Multicomm *cur_comms;
// Failure mutex is locked in shared mode when accessing normal MPI operations
// When a restart operation is started, we need to lock it exclusively
std::shared_timed_mutex failure_mtx;
std::mutex change_world_mtx;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

#define PERIOD 1


void repair_failure() {
    // Failure repair procedure needed - for all ranks
    int old_size, new_size, failed, ranks[LEGIO_MAX_FAILS], i, rank, original_world_size, flag;
    double start, finish;
    MPI_Comm_size(MPI_COMM_WORLD, &original_world_size);
    std::vector<int> failed_world_ranks;
    ComplexComm *world = cur_comms->translate_into_complex(MPI_COMM_WORLD);
    start = MPI_Wtime();
    // Ensure all failed ranks are acked
    // MPIX_Comm_revoke(world->get_comm());
    MPIX_Comm_failure_ack(world->get_comm());
    who_failed(world->get_comm(), &failed, ranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 1) {
        finish = MPI_Wtime();
        FILE *file_p = fopen("output-failure-ack.csv", "a");
        fprintf(file_p, "%f\n", finish-start);
        fclose(file_p);
        start = MPI_Wtime();
    }


    if (failed == 0) {
        // We already repaired, let's exit!
        if (VERBOSE)
                {
                    int rank, size;
                    MPI_Comm_size(MPI_COMM_WORLD, &size);
                    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                    printf("Skipping to repair failure from rank %d / %d\n", rank, size); fflush(stdout);
                }
        return;
    }
    else {
        if (VERBOSE)
                {
                    int rank, size;
                    MPI_Comm_size(MPI_COMM_WORLD, &size);
                    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                    printf("Found a number of process failed (%d) from rank %d / %d\n", failed, rank, size); fflush(stdout);
                }
    }

    MPI_Comm tmp_intracomm, tmp_intercomm, tmp_world, new_world;
    //PMPI_Comm_size(world->get_comm(), &rank);
    PMPIX_Comm_shrink(world->get_comm(), &tmp_world);
    PMPI_Comm_size(tmp_world, &new_size);

    if (rank == 1) {
        finish = MPI_Wtime();
        FILE *file_p = fopen("output-shrink.csv", "a");
        fprintf(file_p, "%f\n", finish-start);
        fclose(file_p);
        start = MPI_Wtime();
    }
    
    // Transform the failed processes to world alias ranks
    std::vector<Rank> world_ranks = cur_comms->get_ranks();
    for (i = 0; i < failed; i++) {
        failed_world_ranks.push_back(cur_comms->untranslate_world_rank(ranks[i]));
        printf("FAILED RANK: %d TRANSLATED %d\n", ranks[i], cur_comms->untranslate_world_rank(ranks[i])); fflush(stdout);  
    }

    // Set the ranks as failed and gather to respawn
    std::vector<int> current_to_respawn;
    for (auto failed_world_rank : failed_world_ranks) {
        if (std::find(cur_comms->to_respawn.begin(), cur_comms->to_respawn.end(), failed_world_rank) == cur_comms->to_respawn.end()) {
            cur_comms->set_failed_rank(failed_world_rank);
        }
        else {
            current_to_respawn.push_back(failed_world_rank);
            printf("I should respawn the following rank: %d\n", failed_world_rank); fflush(stdout);
        }
    }

    std::vector<int> all_failed_ranks;
    for (auto cur_rank : cur_comms->get_ranks()) {
        if (cur_rank.failed) {
            all_failed_ranks.push_back(cur_rank.number);
        }
    }

    // Re-generate all MPI_Comm_world for all the restarted processes
    std::vector<char*> program_names;
    std::vector<char**> argvs;
    std::vector<int> max_procs;
    std::vector<MPI_Info> infos;
    for (auto to_respawn : current_to_respawn) {
        char ** newargv = (char **) malloc(sizeof(char *)*10);
        for (i = 0; i<10; i++) {
            newargv[i] = (char *) malloc(sizeof(char)*30);
        }
        sprintf(newargv[0], "--respawned");
        sprintf(newargv[1], "--rank");
        sprintf(newargv[2], "%d", to_respawn);
        sprintf(newargv[3], "--size");
        sprintf(newargv[4], "%d", original_world_size);
        sprintf(newargv[5], "--to-respawn");
        std::string separator;
        std::ostringstream ss;
        for (auto x : cur_comms->to_respawn) {
            ss << separator << x;
            separator = ",";
        }
        sprintf(newargv[6], "%s", ss.str().c_str());
        
        if (cur_comms->get_ranks().size() != 0) {
            newargv[7] = NULL;
            newargv[8] = NULL;
        } else {
            sprintf(newargv[7], "--failed-ranks");
            std::string separator_failed_ranks;
            std::ostringstream ss_failed_ranks;
            for (auto x : all_failed_ranks) {
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

    if (current_to_respawn.size() != 0) {
        PMPI_Comm_spawn_multiple(current_to_respawn.size(), program_names.data(), argvs.data(), max_procs.data(), infos.data(), 0, tmp_world, &tmp_intercomm, NULL);
        if (rank == 1) {
            finish = MPI_Wtime();
            FILE *file_p = fopen("output-spawn.csv", "a");
            fprintf(file_p, "%f\n", finish-start);
            fclose(file_p);
            start = MPI_Wtime();
        }
        PMPI_Intercomm_merge(tmp_intercomm, 1, &tmp_intracomm);
        PMPI_Comm_split(tmp_intracomm, 1, rank, &new_world);
        if (rank == 1) {
            finish = MPI_Wtime();
            FILE *file_p = fopen("output-repair-world.csv", "a");
            fprintf(file_p, "%f\n", finish-start);
            fclose(file_p);
            start = MPI_Wtime();
        }
        printf("Completed respawn of process from rank %d\n", rank); fflush(stdout);
    }
    else {
        printf("NO RANK TO RESPAWN from process %d!\n", rank); fflush(stdout);
        new_world = tmp_world;
    }
    MPI_Comm_set_errhandler(new_world, MPI_ERRORS_RETURN);
    cur_comms->change_comm(cur_comms->translate_into_complex(MPI_COMM_WORLD), new_world);

    MPI_Group group_world;
    PMPI_Comm_group(new_world , &group_world);
    ComplexComm* complex = cur_comms->translate_into_complex(MPI_COMM_WORLD);

    // Regenerate the supported comms
    for (auto entry: cur_comms->supported_comms_vector) {
        printf("Rank %d is regenerating the supported comm\n", rank);
        auto world_ranks = entry.get_current_world_ranks();
        // Get the group of current world ranks
        std::vector<int> ranks_in_comm, ranks_in_comm_translated;
        MPI_Group new_group;
        MPI_Comm new_comm, alias_comm = entry.alias;
        for (auto rank : world_ranks) {
            if (!rank.failed)
                ranks_in_comm.push_back(rank.number);
        }

        // Translate the ranks according to current world
        for (auto rank : ranks_in_comm) {
            int dest;
            cur_comms->translate_ranks(rank, complex, &dest);
            ranks_in_comm_translated.push_back(dest);
        }

        MPI_Group_incl(group_world, ranks_in_comm_translated.size(), ranks_in_comm_translated.data(), &new_group);

        PMPI_Comm_create(new_world, new_group, &new_comm);
        if (alias_comm != MPI_COMM_NULL) {
            cur_comms->change_comm(cur_comms->get_comm_by_c2f(MPI_Comm_c2f(alias_comm)), new_comm);
        }
        MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
    }

    if (rank == 1) {
        finish = MPI_Wtime();
        FILE *file_p = fopen("output-repair-communicators.csv", "a");
        fprintf(file_p, "%f\n", finish-start);
        fclose(file_p);
        start = MPI_Wtime();
    }

    if (VERBOSE)
    {
        int rank, size;
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        printf("Rank %d / %d: completed respawning process for current failure.\n", rank, size); fflush(stdout);
    }
}

// Initialize a communicator starting from MPI_COMM_WORLD ranks
// If current rank not inside ranks, return NULL
void initialize_comm(int n, const int *ranks, MPI_Comm *newcomm) {
    MPI_Group group_world, new_group;
    int rank, found = false;
    if (!cur_comms->respawned) {
        std::vector<Rank> world_ranks;
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        PMPI_Comm_group( MPI_COMM_WORLD , &group_world);
        for (int i = 0; i < n; i++) {
            world_ranks.push_back(Rank(ranks[i], false));
        }
        PMPI_Group_incl(group_world, n, ranks, &new_group);
        MPI_Comm_create(MPI_COMM_WORLD, new_group, newcomm);
        if (*newcomm != MPI_COMM_NULL) {
            ComplexComm complex_comm = *cur_comms->translate_into_complex(*newcomm);
            cur_comms->supported_comms.insert({complex_comm.get_alias_id(), cur_comms->supported_comms_vector.size()});
        }
        
        SupportedComm* supported_comm = new SupportedComm(*newcomm, world_ranks);
        cur_comms->supported_comms_vector.push_back(*supported_comm);
    } else {
        std::vector<Rank> world_ranks;
        int size;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        ComplexComm* world_complex = cur_comms->translate_into_complex(MPI_COMM_WORLD);
        MPI_Comm world = world_complex->get_comm();
        RespawnMulticomm* respawned_comms = dynamic_cast<RespawnMulticomm*>(cur_comms);

        // Get the group of current world ranks
        std::vector<int> ranks_in_comm, ranks_in_comm_translated;
        MPI_Group new_group;
        MPI_Comm new_comm;
        for (int i = 0; i < n; i++) {
            if (!cur_comms->get_ranks().at(ranks[i]).failed) {
                ranks_in_comm.push_back(i);
                world_ranks.push_back(Rank(ranks[i], false));
            }
            else {
                world_ranks.push_back(Rank(ranks[i], true));
            }
        }

        ComplexComm *complex = cur_comms->translate_into_complex(MPI_COMM_WORLD);
        // Translate the ranks according to current world
        int dest = 0;
        for (auto rank : ranks_in_comm) {
            printf("Pushing comm %d\n\n", rank);
            cur_comms->translate_ranks(rank, complex, &dest);
            ranks_in_comm_translated.push_back(dest);
        }

        MPI_Comm_group(complex->get_comm(), &group_world);
        PMPI_Group_incl(group_world, ranks_in_comm_translated.size(), ranks_in_comm_translated.data(), &new_group);
        printf("Rank %d is re-creating the comm\n", rank);
        PMPI_Comm_create(complex->get_comm(), new_group, &new_comm);


        MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
        cur_comms->add_comm(
            new_comm,
            MPI_COMM_WORLD,
            [new_group] (MPI_Comm source, MPI_Comm *dest) -> int 
            {
                int rc = PMPI_Comm_create_group(source, new_group, 1, dest);
                MPI_Comm_set_errhandler(*dest, MPI_ERRORS_RETURN);
                return rc;
            });

        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(new_comm, &size);
            PMPI_Comm_rank(new_comm, &rank);
            printf("Rank %d / %d: completed group restoration.\n", rank, size); fflush(stdout);
        }
        *newcomm = new_comm;

        SupportedComm* supported_comm = new SupportedComm(*newcomm, world_ranks);
        cur_comms->supported_comms.insert({cur_comms->translate_into_complex(new_comm)->get_alias_id(), cur_comms->supported_comms_vector.size()});
        cur_comms->supported_comms_vector.push_back(*supported_comm);
    }
}

void loop_repair_failures()
{
    int rank;
    
    while(1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        int length;
        MPI_Status status;
        MPI_Comm world_comm;
        change_world_mtx.lock();
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        ComplexComm *world = cur_comms->translate_into_complex(MPI_COMM_WORLD);
        world_comm = world->get_comm();
        MPI_Comm_set_errhandler(world_comm, MPI_ERRORS_RETURN);
        if (world_comm == MPI_COMM_NULL) {
            change_world_mtx.unlock();
            return;
        }
        //printf("Probing from rank %d\n", rank);
        int flag = 0, flag_self = 0, buf;
        int local_rank;
        PMPI_Iprobe(MPI_ANY_SOURCE, LEGIO_FAILURE_TAG, world_comm, &flag, MPI_STATUS_IGNORE);
        if(flag)
        {
            //failure_mtx.lock();
            int rank, size;
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            printf("\n\n\n%d FOUND SOMETHING TO REPAIR THROUGH THREAD!\n", rank);
            PMPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, LEGIO_FAILURE_TAG, world_comm, &status);
            if (VERBOSE)
                {
                    printf("Rank %d / %d: received failure notification from %d.\n", rank, size, status.MPI_SOURCE); fflush(stdout);
                }
            change_world_mtx.unlock();
            failure_mtx.lock();
            repair_failure();
            failure_mtx.unlock();
        }
        else
            change_world_mtx.unlock();
    }
}


void restart(int rank)
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
    cur_comms->change_comm(cur_comms->translate_into_complex(MPI_COMM_WORLD), new_world);

    if (VERBOSE)
        {
            printf("Rank %d respawn complete.\n", rank); fflush(stdout);
        }
}


bool is_respawned() {
    return cur_comms->respawned;
}
