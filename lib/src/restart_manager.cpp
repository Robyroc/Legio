#include "restart_manager.hpp"
#include "config.hpp"

using namespace legio;

// Shift a translated rank considering the failed ranks inside the world
int RestartManager::untranslate_world_rank(int translated)
{
    assert(initialized);
    // 1 2 3 4
    // 1 x 3 4
    // 2 = translated -> needs to become three
    int source = translated;
    for (int i = 0; i <= translated; i++)
        if (ranks.at(i).failed)
            source++;
    return source;
}

void RestartManager::set_failed_rank(int world_rank)
{
    ranks.at(world_rank).failed = true;
    for (auto& supported_comm : supported_comms)
    {
        supported_comms_vector[supported_comm.second].set_failed(world_rank);
    }
}

// Translate ranks from source (current ranks) to destination (alias-related ranks)
int RestartManager::translate_ranks(int source_rank, ComplexComm& comm)
{
    assert(initialized);
    MPI_Group tr_group;
    int source = source_rank;
    int failed_ranks = 0;
    auto res = supported_comms.find(comm.get_alias_id());
    if (res == supported_comms.end() && comm.get_alias() != MPI_COMM_WORLD)
    {
        MPI_Group tr_group;
        int source = source_rank, dest_rank;
        MPI_Comm_group(comm.get_comm(), &tr_group);
        MPI_Group_translate_ranks(comm.get_group(), 1, &source, tr_group, &dest_rank);
        return dest_rank;
    }
    else if (comm.get_alias() == MPI_COMM_WORLD)
    {
        for (int i = 0; i < source_rank; i++)
            if (ranks.at(i).failed)
                failed_ranks++;
        return source_rank - failed_ranks;
    }
    else
    {
        SupportedComm respawned_comm = supported_comms_vector[res->second];
        failed_ranks = respawned_comm.get_failed_ranks_before(source_rank);
        return source_rank - failed_ranks;
    }
}

// Multicomm::Multicomm(int size)
void RestartManager::initialize(const int size)
{
    const std::lock_guard<std::mutex> lock(init_lock);
    if (initialized)
        return;
    initialized = true;
    respawned = false;
    // Initialize ranks
    for (int i = 0; i < size; i++)
    {
        ranks.push_back(Rank(i, false));
    }
};

void RestartManager::initialize(const int size, const int rank_, const std::vector<int> failed)
{
    if constexpr (!BuildOptions::with_restart)
        assert(false && "Unsupported (recompile with restart)");
    const std::lock_guard<std::mutex> lock(init_lock);
    if (initialized)
        return;
    initialized = true;
    respawned = true;
    for (int i = 0; i < size; i++)
    {
        bool is_failed = std::count(failed.begin(), failed.end(), i) != 0;
        ranks.push_back(Rank(i, is_failed));
    }
    own_rank = rank_;
}
const bool RestartManager::is_initialized()
{
    const std::lock_guard<std::mutex> lock(init_lock);
    return initialized;
}