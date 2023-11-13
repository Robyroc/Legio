#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <numeric>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "config.hpp"
#include "context.hpp"
#include "mpi-ext.h"
#include "mpi.h"
//#include "respawn_multicomm.hpp"
#include "supported_comm.hpp"
extern "C" {
#include "legio.h"
#include "restart.h"
}
//#include <thread>

#define PERIOD 1
using namespace legio;

// Initialize a communicator starting from MPI_COMM_WORLD ranks
// If current rank not inside ranks, return NULL
void initialize_comm(const int n, const int* ranks, MPI_Comm* newcomm)
{
    if constexpr (!BuildOptions::with_restart)
    {
        assert(false && "Unsupported (recompile with restart)");
    }
    else
    {
        MPI_Group group_world, new_group;
        int rank, found = false;
        if (!Context::get().r_manager.is_respawned())
        {
            std::vector<Rank> world_ranks;
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            PMPI_Comm_group(MPI_COMM_WORLD, &group_world);
            for (int i = 0; i < n; i++)
            {
                world_ranks.push_back(Rank(ranks[i], false));
            }
            PMPI_Group_incl(group_world, n, ranks, &new_group);
            MPI_Comm_create(MPI_COMM_WORLD, new_group, newcomm);
            if (*newcomm != MPI_COMM_NULL)
            {
                ComplexComm& complex_comm = Context::get().m_comm.translate_into_complex(*newcomm);
                Context::get().r_manager.add_to_supported_comms(
                    {complex_comm.get_alias_id(),
                     Context::get().r_manager.supported_comms_vector.size()});
            }

            SupportedComm* supported_comm = new SupportedComm(*newcomm, world_ranks);
            Context::get().r_manager.supported_comms_vector.push_back(*supported_comm);
        }
        else
        {
            std::vector<Rank> world_ranks;
            int size;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            ComplexComm& world_complex =
                Context::get().m_comm.translate_into_complex(MPI_COMM_WORLD);
            MPI_Comm world = world_complex.get_comm();
            // RespawnMulticomm* respawned_comms = dynamic_cast<RespawnMulticomm*>(cur_comms);

            // Get the group of current world ranks
            std::vector<int> ranks_in_comm, ranks_in_comm_translated;
            MPI_Group new_group;
            MPI_Comm new_comm;
            for (int i = 0; i < n; i++)
            {
                if (!Context::get().r_manager.get_ranks().at(ranks[i]).failed)
                {
                    ranks_in_comm.push_back(i);
                    world_ranks.push_back(Rank(ranks[i], false));
                }
                else
                {
                    world_ranks.push_back(Rank(ranks[i], true));
                }
            }

            ComplexComm& complex = Context::get().m_comm.translate_into_complex(MPI_COMM_WORLD);
            // Translate the ranks according to current world
            int dest = 0;
            for (auto rank : ranks_in_comm)
            {
                /*
                printf("Pushing comm %d\n\n", rank);
                */
                dest = Context::get().r_manager.translate_ranks(rank, complex);
                ranks_in_comm_translated.push_back(dest);
            }

            MPI_Comm_group(complex.get_comm(), &group_world);
            PMPI_Group_incl(group_world, ranks_in_comm_translated.size(),
                            ranks_in_comm_translated.data(), &new_group);
            /*
            printf("Rank %d is re-creating the comm\n", rank);
            */
            PMPI_Comm_create(complex.get_comm(), new_group, &new_comm);

            MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
            Context::get().m_comm.add_comm(new_comm);
            /*
            if (VERBOSE)
            {
                int rank, size;
                PMPI_Comm_size(new_comm, &size);
                PMPI_Comm_rank(new_comm, &rank);
                printf("Rank %d / %d: completed group restoration.\n", rank, size);
                fflush(stdout);
            }
            */
            *newcomm = new_comm;

            SupportedComm* supported_comm = new SupportedComm(*newcomm, world_ranks);
            Context::get().r_manager.add_to_supported_comms(
                {Context::get().m_comm.translate_into_complex(new_comm).get_alias_id(),
                 Context::get().r_manager.supported_comms_vector.size()});
            Context::get().r_manager.supported_comms_vector.push_back(*supported_comm);
        }
    }
}

int is_respawned()
{
    return Context::get().r_manager.is_respawned();
}

void add_critical(int rank)
{
    if constexpr (!BuildOptions::with_restart)
    {
        assert(false && "Unsupported (recompile with restart)");
    }
    else
        Context::get().r_manager.add_to_respawn_list(rank);
}