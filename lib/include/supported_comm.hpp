#ifndef SUPPORTED_COMM_HPP
#define SUPPORTED_COMM_HPP

#include <vector>
#include "mpi.h"

namespace legio {

class Rank
{
   public:
    Rank(int number, bool failed);
    bool failed;
    // The rank based on MPI_COMM_WORLD
    int number;
};

class SupportedComm
{
   public:
    SupportedComm(MPI_Comm comm, std::vector<Rank> world_ranks);
    int get_failed_ranks_before(int rank);
    MPI_Comm get_alias() { return alias; };

    std::vector<Rank> get_current_world_ranks() { return world_ranks; }
    void set_failed(int world_rank)
    {
        for (auto& rank : world_ranks)
            if (rank.number == world_rank)
            {
                rank.failed = true;
                break;
            }
    }

    MPI_Comm alias;
    // Ordered vector with the world ranks
    std::vector<Rank> world_ranks;
};

class RespawnedSupportedComm : public SupportedComm
{
   public:
    int size();
    int rank();
};

}  // namespace legio

#endif