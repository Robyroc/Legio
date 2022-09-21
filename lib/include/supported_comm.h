#ifndef SUPPORTED_COMM_H
#define SUPPORTED_COMM_H

#include "mpi.h"
#include <list>
#include <vector>
#include <set>
#include <functional>

class SupportedComm
{
    public:
        MPI_Comm get_alias() {return alias;};
        SupportedComm(MPI_Comm comm, std::set<int> current_world_ranks);

        std::set<int> get_current_world_ranks() {
            return current_world_ranks;
        }

    private:
        MPI_Comm alias;
        std::set<int> current_world_ranks;
};

class RespawnedSupportedComm : SupportedComm
{
    public:
        int size();
        int rank();
        int get_failed_ranks_before(int rank);
        RespawnedSupportedComm(MPI_Comm comm, std::set<int> current_world_ranks, std::set<int> failed_ranks);

    private:
        std::set<int> failed_ranks;

};

#endif