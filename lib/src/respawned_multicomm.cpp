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

RespawnedMulticomm::RespawnedMulticomm()
{}

void RespawnedMulticomm::translate_ranks(int source_rank, ComplexComm* comm, int* dest_rank) {
    int alias_id = comm->get_alias_id();
    RespawnedSupportedComm supported_comm = supported_comms.find(alias_id)->second;

    int failed_ranks_before_source = supported_comm.get_failed_ranks_before(source_rank);
    *dest_rank = source_rank - failed_ranks_before_source;
};
