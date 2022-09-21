#include "mpi.h"
#include "mpi-ext.h"
#include "supported_comm.h"
#include "complex_comm.h"
#include "respawned_multicomm.h"
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
#include <set>

SupportedComm::SupportedComm(MPI_Comm alias, std::set<int> current_world_ranks) {
    alias = alias;
    current_world_ranks = current_world_ranks;
}

RespawnedSupportedComm::RespawnedSupportedComm(MPI_Comm alias, std::set<int> current_world_ranks, std::set<int> failed_ranks)
{
    alias = alias;
    current_world_ranks = current_world_ranks;
    failed_ranks = failed_ranks;
}

int RespawnedSupportedComm::size() {
    int size;
    MPI_Comm_size( get_alias() , &size);

    return size + failed_ranks.size();
}



int RespawnedSupportedComm::size() {
    int rank, i;
    int size = RespawnedSupportedComm::size();
    MPI_Comm_rank(get_alias(), &rank);

    // Reconstruct the original rank by iterating over the ranks of the original comunicator
    while (rank != 0) {
        if (std::find(failed_ranks.begin(), failed_ranks.end(), i) == failed_ranks.end()) {
            // Rank is not failed, count it
            rank--;
        }
    }

    return i;
}


int RespawnedSupportedComm::get_failed_ranks_before(int rank) {
    return std::count_if(failed_ranks.begin(), failed_ranks.end(), [rank] (int failed_rank) {
        return failed_rank < rank;
    });
}