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
#include <set>

extern Multicomm *cur_comms;

Rank::Rank(int number, bool failed) {
    number = number;
    failed = failed;
}

SupportedComm::SupportedComm(MPI_Comm alias, std::vector<Rank> world_ranks) {
    alias = alias;
    world_ranks = world_ranks;
}

int RespawnedSupportedComm::size() {
    return world_ranks.size();
}


int RespawnedSupportedComm::rank() {
    int rank, dest;
    int size = RespawnedSupportedComm::size();
    MPI_Comm_rank(get_alias(), &rank);

    cur_comms->translate_ranks(rank, cur_comms->translate_into_complex(alias), &dest);
    return dest;
}

int SupportedComm::get_failed_ranks_before(int rank) {
    int failed = 0;
    for (int i = 0; i < rank; i++) {
        if (world_ranks.at(i).failed)
            i++;
    }
    // Cache it and clean the cache after
    return failed;
}
