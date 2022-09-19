#ifndef SUPPORTED_COMM_H
#define SUPPORTED_COMM_H

#include "mpi.h"
#include <list>
#include <vector>
#include <unordered_map>
#include <functional>

class SupportedComm
{
    public:
        MPI_Comm get_alias() {return alias;};
        SupportedComm(MPI_Comm comm);

    private:
        MPI_Comm alias;
};

class RespawnedSupportedComm : SupportedComm
{
    public:
        int size();
        int rank();
        int get_failed_ranks_before(int rank);
        RespawnedSupportedComm(MPI_Comm comm, std::vector<int> failed_ranks);

    private:
        std::vector<int> failed_ranks;

};

#endif