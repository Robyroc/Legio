#ifndef RESTART_MANAGER_HPP
#define RESTART_MANAGER_HPP

#include <list>
#include <map>
#include <mutex>
#include <vector>
#include "complex_comm.hpp"
#include "mpi.h"
#include "supported_comm.hpp"

namespace legio {

class RestartManager
{
   public:
    RestartManager(RestartManager const&) = delete;
    RestartManager& operator=(RestartManager const&) = delete;
    RestartManager(RestartManager&&) = default;
    RestartManager& operator=(RestartManager&&) = default;
    RestartManager() = default;

    std::vector<SupportedComm> supported_comms_vector;
    void set_failed_rank(int world_rank);
    int translate_ranks(int, ComplexComm&);
    int untranslate_world_rank(int translated);

    inline std::vector<Rank> get_ranks()
    {
        assert(initialized);
        return ranks;
    }

    inline const std::map<int, RespawnedSupportedComm>& access_supported_comms_respawned()
    {
        assert(initialized);
        assert(respawned);
        return supported_comms_respawned;
    };

    inline const std::vector<int> get_respawn_list() const
    {
        assert(initialized);
        return to_respawn;
    };

    inline void add_to_respawn_list(const int rank) { to_respawn.push_back(rank); }

    inline const int& get_own_rank() const
    {
        assert(initialized);
        assert(respawned);
        return own_rank;
    }

    inline const bool is_respawned() const
    {
        if constexpr (BuildOptions::with_restart)
        {
            assert(initialized);
            return respawned;
        }
        else
            return false;
    }

    inline void add_to_supported_comms(std::pair<int, int> addee)
    {
        assert(initialized);
        supported_comms.insert(addee);
    }

    void initialize(const int size);
    void initialize(const int size, const int rank_, const std::vector<int> failed);
    const bool is_initialized();

   private:
    bool respawned = false;
    bool initialized = false;
    std::mutex init_lock;
    std::vector<Rank> ranks;
    std::map<int, RespawnedSupportedComm> supported_comms_respawned;
    int own_rank;
    std::vector<int> to_respawn;
    std::map<int, int> supported_comms;
};

}  // namespace legio

#endif