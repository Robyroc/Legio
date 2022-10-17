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

RespawnMulticomm::RespawnMulticomm(int size, int rank_, std::vector<int> failed) {
    // Initialize ranks
    for (int i = 0; i < size; i++) {
        bool is_failed = std::count(failed.begin(), failed.end(), i) != 0;
        ranks.push_back(Rank(i, is_failed));
    }
    rank = rank_;
}

void RespawnMulticomm::translate_ranks(int source_rank, ComplexComm* comm, int* dest_rank)
{
    Multicomm::translate_ranks(source_rank, comm, dest_rank);
}
