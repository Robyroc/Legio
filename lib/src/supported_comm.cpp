#include "supported_comm.hpp"
#include "complex_comm.hpp"
#include "mpi.h"
#include "multicomm.hpp"
// #include "respawn_multicomm.h"

using namespace legio;

Rank::Rank(int number_, bool failed_)
{
    number = number_;
    failed = failed_;
}

SupportedComm::SupportedComm(MPI_Comm alias_, std::vector<Rank> world_ranks_)
{
    alias = alias_;
    world_ranks = world_ranks_;
}

int RespawnedSupportedComm::size()
{
    return world_ranks.size();
}

int RespawnedSupportedComm::rank()
{
    int rank, dest;
    int size = RespawnedSupportedComm::size();
    MPI_Comm_rank(get_alias(), &rank);

    return Multicomm::get_instance().translate_ranks(
        rank, Multicomm::get_instance().translate_into_complex(alias));
}

int SupportedComm::get_failed_ranks_before(int rank)
{
    int failed = 0;
    for (int i = 0; i < rank; i++)
    {
        if (world_ranks.at(i).failed)
            failed++;
    }
    // Cache it and clean the cache after
    return failed;
}
