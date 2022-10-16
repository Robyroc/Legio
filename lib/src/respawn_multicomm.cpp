#include "mpi.h"
#include "mpi-ext.h"
#include "supported_comm.h"
#include "complex_comm.h"
#include "respawn_multicomm.h"
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

RespawnMulticomm::RespawnMulticomm(int size, std::vector<int> failed) {
    // Initialize ranks
    for (int i = 0; i < size; i++) {
        bool is_failed = std::count(failed.begin(), failed.end(), i) != 0;
        ranks.push_back(Rank(i, is_failed));
    }
}

void RespawnMulticomm::translate_ranks(int source_rank, ComplexComm* comm, int* dest_rank)
{
    MPI_Group tr_group;
    int source = source_rank;
    auto res = supported_comms.find(comm->get_alias_id());
    if( res == supported_comms.end()) {
        Multicomm::translate_ranks(source_rank, comm, dest_rank);
    }
    RespawnedSupportedComm respawned_comm = res->second;

    int failed_ranks = respawned_comm.get_failed_ranks_before(source_rank);
    *dest_rank = *dest_rank - failed_ranks;
}
