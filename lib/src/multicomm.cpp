#include "multicomm.hpp"
#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include "complex_comm.hpp"
#include "config.hpp"
#include "mpi.h"

using namespace legio;

// Multicomm::Multicomm(int size)
void Multicomm::initialize(int size)
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

void Multicomm::initialize(const int size, const int rank_, const std::vector<int> failed)
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
const bool Multicomm::is_initialized()
{
    const std::lock_guard<std::mutex> lock(init_lock);
    return initialized;
}

MPI_Comm Multicomm::get_horizon_comm(MPI_Group group)
{
    const std::lock_guard<std::mutex> lock(horizon_lock);
    for (auto horizon : horizon_comms)
    {
        MPI_Group horizon_group, diff;
        int size;
        PMPI_Comm_group(horizon, &horizon_group);
        PMPI_Group_difference(group, horizon_group, &diff);
        PMPI_Group_size(diff, &size);
        PMPI_Group_free(&horizon_group);
        PMPI_Group_free(&diff);
        if (!size)
            return horizon;
    }
    return MPI_COMM_NULL;
}

void Multicomm::add_horizon_comm(MPI_Comm comm)
{
    const std::lock_guard<std::mutex> lock(horizon_lock);
    std::list<MPI_Comm> new_horizon;
    for (std::list<MPI_Comm>::iterator horizon = horizon_comms.begin();
         horizon != horizon_comms.end(); horizon++)
    {
        MPI_Group horizon_group, comm_group, diff_hc, diff_ch;
        int size_hc, size_ch;
        PMPI_Comm_group(*horizon, &horizon_group);
        PMPI_Comm_group(comm, &comm_group);
        PMPI_Group_difference(horizon_group, comm_group, &diff_hc);
        PMPI_Group_difference(comm_group, horizon_group, &diff_ch);
        PMPI_Group_size(diff_hc, &size_hc);
        PMPI_Group_size(diff_ch, &size_ch);
        PMPI_Group_free(&horizon_group);
        PMPI_Group_free(&comm_group);
        PMPI_Group_free(&diff_hc);
        PMPI_Group_free(&diff_ch);
        if (!size_ch)
        {
            // Group is contained by the horizon
            new_horizon.splice(new_horizon.end(), horizon_comms, horizon, horizon_comms.end());
            horizon_comms = new_horizon;
            return;
        }
        if (size_hc)
        {
            // Horizon is not contained by group
            new_horizon.push_back(*horizon);
        }
    }
    new_horizon.push_back(comm);
    horizon_comms = new_horizon;
    return;
}

int Multicomm::add_comm(MPI_Comm added)
{
    assert(initialized);
    if (!respawned)
    {
        int id = c2f<MPI_Comm>(added);
        MPI_Comm temp;
        PMPI_Comm_dup(added, &temp);
        MPI_Comm_set_errhandler(temp, MPI_ERRORS_RETURN);
        std::pair<int, ComplexComm> adding(id, ComplexComm(temp, id));
        auto res = comms.insert(adding);
        return res.second;
    }
    else
    {
        int id = c2f<MPI_Comm>(added);
        std::pair<int, ComplexComm> adding(id, ComplexComm(added, id));
        auto res = comms.insert(adding);
        return res.second;
    }
}

ComplexComm& Multicomm::translate_into_complex(MPI_Comm input)
{
    assert(initialized);
    auto res = comms.find(c2f<MPI_Comm>(input));
    if (res == comms.end())
    {
        if (input != MPI_COMM_NULL)
            assert(false && "THIS SHOULDN'T HAVE HAPPENED, USE part_of BEFORE TRANSLATE.\n");
        throw std::invalid_argument("USE part_of BEFORE TRANSLATE.\n");
    }
    else
        return res->second;
}

void Multicomm::remove(MPI_Comm removed, std::function<int(MPI_Comm*)> destroyer)
{
    assert(initialized);
    int id = c2f<MPI_Comm>(removed);
    auto res = comms.find(id);
    // std::unordered_map<int, int>::iterator res = comms_order.find(id);
    if (res != comms.end())
    {
        MPI_Comm target = res->second.get_comm();
        // destroyer(&target);
        // Removed deletion since it may be useful for the recreation of the following comms
        comms.erase(id);
    }
    else
    {
        assert(false && "THIS SHOULDN'T HAVE HAPPENED, REMOVING A NEVER INSERTED COMM.\n");
    }
}

const bool Multicomm::part_of(MPI_Comm checked) const
{
    assert(initialized);
    auto res = comms.find(c2f<MPI_Comm>(checked));
    return res != comms.end();
}

void Multicomm::remove_structure(MPI_Win* win)
{
    assert(initialized);
    if (part_of(*win))
    {
        ComplexComm& translated = get_complex_from_structure(*win);
        translated.remove_structure(*win);
        maps[handle_selector<MPI_Win>::get()].erase(MPI_Win_c2f(*win));
    }
    else
        PMPI_Win_free(win);
}

void Multicomm::remove_structure(MPI_File* file)
{
    assert(initialized);
    if (part_of(*file))
    {
        ComplexComm& translated = get_complex_from_structure(*file);
        translated.remove_structure(*file);
        maps[handle_selector<MPI_File>::get()].erase(MPI_File_c2f(*file));
    }
    else
        PMPI_File_close(file);
}

void Multicomm::remove_structure(MPI_Request* req)
{
    assert(initialized);
    if (part_of(*req))
    {
        ComplexComm& translated = get_complex_from_structure(*req);
        translated.remove_structure(*req);
        maps[handle_selector<MPI_Request>::get()].erase(c2f<MPI_Request>(*req));
    }
}

std::vector<Rank> Multicomm::get_ranks()
{
    assert(initialized);
    return ranks;
}

// Shift a translated rank considering the failed ranks inside the world
int Multicomm::untranslate_world_rank(int translated)
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

void Multicomm::set_failed_rank(int world_rank)
{
    ranks.at(world_rank).failed = true;
    for (auto& supported_comm : supported_comms)
    {
        supported_comms_vector[supported_comm.second].set_failed(world_rank);
    }
}

// void Multicomm::change_comm(ComplexComm &current, MPI_Comm newcomm)
// {
//     assert(!initialized);
//     current.replace_comm(newcomm);
// }

// Translate ranks from source (current ranks) to destination (alias-related ranks)
int Multicomm::translate_ranks(int source_rank, ComplexComm& comm)
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
