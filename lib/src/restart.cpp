#include "mpi.h"
#include "mpi-ext.h"
#include "comm_manipulation.h"
#include "complex_comm.h"
#include "respawned_multicomm.h"
#include "multicomm.h"
#include "legio.h"
#include <functional>
#include <chrono>
#include <string>
#include <shared_mutex>
#include <thread>
#include <condition_variable>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <sstream>
#include <algorithm>
//#include <thread>

extern Multicomm *cur_comms;
// Failure mutex is locked in shared mode when accessing normal MPI operations
// When a restart operation is started, we need to lock it exclusively
std::shared_timed_mutex failure_mtx;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

#define PERIOD 1


void repair_failure() {
    // Failure repair procedure needed - for all ranks
    int old_size, new_size, failed, ranks[LEGIO_MAX_FAILS], i, rank;
    ComplexComm *world = cur_comms->translate_into_complex(MPI_COMM_WORLD);

    MPIX_Comm_failure_ack(world->get_comm());
    who_failed(world->get_comm(), &failed, ranks);
    PMPI_Comm_rank(world->get_comm(), &rank);

    if (failed == 0) {
        // We already repaired, let's exit!
        return;
    }

    MPI_Comm tmp_intracomm, tmp_intercomm, tmp_world, new_world;
    PMPIX_Comm_shrink(world->get_comm(), &tmp_world);

    PMPI_Comm_size(tmp_world, &new_size);

    // Translate the processes to respawn after the failure
    std::vector<int> all_to_respawn = cur_comms->to_respawn;
    for (i = 0; i < failed && std::find(cur_comms->to_respawn.begin(), cur_comms->to_respawn.end(), ranks[i]) == cur_comms->to_respawn.end(); i++) {
        std::transform(all_to_respawn.begin(), all_to_respawn.end(), all_to_respawn.begin(), [&ranks](int &i) {
            if (i > ranks[i])
                return i-1;
            return i;
        });
    }

    // Iterate over failed processes to gather the ones to respawn
    std::vector<int> failed_to_respawn;
    std::copy_if(&ranks[0], 
        &ranks[failed], 
        std::back_inserter(failed_to_respawn), [](auto val){ 
            return std::find(cur_comms->to_respawn.begin(), cur_comms->to_respawn.end(), val) != cur_comms->to_respawn.end(); 
        }
    );
    if (failed_to_respawn.size() == 0) {
        MPI_Comm_set_errhandler(tmp_world, MPI_ERRORS_RETURN);
        cur_comms->change_comm(cur_comms->translate_into_complex(MPI_COMM_WORLD), tmp_world);
        return;
    }
    for (i = 0; i < failed && std::find(cur_comms->to_respawn.begin(), cur_comms->to_respawn.end(), ranks[i]) == cur_comms->to_respawn.end(); i++) {
        std::transform(failed_to_respawn.begin(), failed_to_respawn.end(), failed_to_respawn.begin(), [&ranks](int &i) {
            if (i > ranks[i])
                return i-1;
            return i;
        });

        // Fixup the current rank too
        if (rank > ranks[i]) {
            rank--;
        }

        cur_comms->add_failed_ranks(ranks[i]);
    }

    // Set the new list of comms to respawn
    cur_comms->to_respawn = all_to_respawn;

    // Re-generate all MPI_Comm_world for all the restarted processes
    std::vector<char*> program_names;
    std::vector<char**> argvs;
    std::vector<int> max_procs;
    std::vector<MPI_Info> infos;                
    for (auto to_respawn : failed_to_respawn) {

        char ** newargv = (char **) malloc(sizeof(char *)*5);
        for (i = 0; i<5; i++) {
            newargv[i] = (char *) malloc(sizeof(char)*20);
        }
        sprintf(newargv[0], "--respawned");
        sprintf(newargv[1], "--rank");
        sprintf(newargv[2], "%d", to_respawn);
        sprintf(newargv[3], "--to-respawn");
        std::string separator;
        std::ostringstream ss;
        for (auto x : cur_comms->to_respawn) {
            ss << separator << x;
        separator = ",";
        }
        sprintf(newargv[3], "%s", ss.str().c_str());
        newargv[4] = NULL;
        // TODO Add list of failed ranks

        argvs.push_back(newargv);
        program_names.push_back(program_invocation_name);
        max_procs.push_back(1);
        infos.push_back(MPI_INFO_NULL);
    }

    PMPI_Comm_spawn_multiple(failed_to_respawn.size(), program_names.data(), argvs.data(), max_procs.data(), infos.data(), 0, tmp_world, &tmp_intercomm, NULL);
    PMPI_Intercomm_merge(tmp_intercomm, 1, &tmp_intracomm);
    PMPI_Comm_split(tmp_intracomm, 1, rank, &new_world);
    MPI_Comm_set_errhandler(new_world, MPI_ERRORS_RETURN);
    cur_comms->change_comm(cur_comms->translate_into_complex(MPI_COMM_WORLD), new_world);

    // Regenerate the supported comms
    for (auto entry: cur_comms->supported_comms) {
        ComplexComm* comm = cur_comms->translate_into_complex(entry.second.get_alias());
        // TODO Regenrate the communicators by recreating the group completely from the initial ranks
        // TODO Change the supported comm updating the failed ranks
        // TODO Replace the complex comm
        // TODO If group has size 0, continue
        // MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
    }

    if (VERBOSE)
    {
        int rank, size;
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        printf("Rank %d / %d: completed respawning process for current failure.\n", rank, size);
    }
}

// Initialize a communicator starting from MPI_COMM_WORLD ranks
void initialize_comm(int n, const int *ranks, MPI_Comm *newcomm) {
    MPI_Group group_world, new_group;
    if (!cur_comms->respawned) {
        MPI_Comm_group( MPI_COMM_WORLD , &group_world);
        MPI_Group_incl(group_world, len, ranks, &new_group);
        MPI_Comm_create_group(MPI_COMM_WORLD, new_group, 1, newcomm);
        ComplexComm complex_comm = *cur_comms->translate_into_complex(*newcomm);
        
        SupportedComm* supported_comm = new SupportedComm(*newcomm);
        cur_comms->supported_comms.insert({complex_comm.get_alias_id(), *supported_comm});
    } else {
        MPI_Comm world = cur_comms->translate_into_complex(MPI_COMM_WORLD)->get_comm();
        RespawnedMulticomm* respawned_comms = dynamic_cast<RespawnedMulticomm*>(cur_comms);
        // Remove from the group the failed ranks
        std::set<int> world_failed_ranks = respawned_comms->get_failed_ranks();
        std::vector<int> input_ranks, remaining_ranks, failed_ranks, remaining_ranks_transformed, failed_ranks_transformed;
        input_ranks.insert(input_ranks.begin(), ranks, ranks+n);

        std::set_difference(input_ranks.begin(), input_ranks.end(), world_failed_ranks.begin(), world_failed_ranks.end(), std::inserter(remaining_ranks, remaining_ranks.begin()));
        std::set_intersection(input_ranks.begin(), input_ranks.end(), world_failed_ranks.begin(), world_failed_ranks.end(), std::inserter(failed_ranks, failed_ranks.begin()));

        for (int i = 0; i < n; i++) {
            int current_rank = ranks[i];

            if (std::count(world_failed_ranks.begin(), world_failed_ranks.end(), current_rank) == 0) {
                // Not failed rank, should be added
                current_rank = current_rank - std::count_if(failed_ranks.begin(), failed_ranks.end(), [current_rank] (int failed_rank) {
                        return failed_rank < current_rank;
                });
                remaining_ranks_transformed.push_back(current_rank);
            }
            else {
                failed_ranks_transformed.push_back(i);
            }
        }

        MPI_Comm_group(world , &group_world);
        MPI_Group_incl(group_world, remaining_ranks_transformed.size(), remaining_ranks_transformed.data(), &new_group);
        MPI_Comm_create_group(world, new_group, 1, newcomm);

        ComplexComm complex_comm = *cur_comms->translate_into_complex(*newcomm);
        RespawnedSupportedComm* supported_comm = new RespawnedSupportedComm(*newcomm, failed_ranks_transformed);
        respawned_comms->supported_comms.insert({complex_comm.get_alias_id(), *supported_comm});
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(world, &size);
            PMPI_Comm_rank(world, &rank);
            printf("Rank %d / %d: completed group restoration.\n", rank, size);
        }
    }
}

void loop_repair_failures()
{
    int rank;
    
    while(1)
    {
        
        int buf, length;
        MPI_Status status;
        ComplexComm *world = cur_comms->translate_into_complex(MPI_COMM_WORLD);
        PMPI_Comm_rank(world->get_comm(), &rank);
        MPI_Comm_set_errhandler(world->get_comm(), MPI_ERRORS_RETURN);
        if (world->get_comm() == MPI_COMM_NULL) {
            return;
        }
        PMPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, LEGIO_FAILURE_TAG, world->get_comm(), &status);

        if (buf == LEGIO_FAILURE_REPAIR_VALUE) {
            if (VERBOSE)
                {
                    int rank, size;
                    PMPI_Comm_size(world->get_comm(), &size);
                    PMPI_Comm_rank(world->get_comm(), &rank);
                    printf("Rank %d / %d: received failure notification from %d.\n", size, rank, status.MPI_SOURCE); fflush(stdout);
                }
            failure_mtx.lock();
            repair_failure();
            failure_mtx.unlock();
        }
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

void MPI_Comm_translate_ranks( MPI_Comm comm1 , int n , const int ranks1[] , MPI_Comm comm2 , int ranks2[]) {
    MPI_Group group1, group2;

    PMPI_Comm_group(comm1, &group1);
    PMPI_Comm_group(comm2, &group2);

    MPI_Group_translate_ranks(group1, n, ranks1, group2, ranks2);
}


bool is_respawned() {
    return cur_comms->respawned;
}