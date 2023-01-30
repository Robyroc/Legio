#ifndef MULTICOMM_HPP
#define MULTICOMM_HPP

#include <assert.h>
#include <stdio.h>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>
#include "complex_comm.hpp"
#include "mpi.h"
#include "struct_selector.hpp"
#include "supported_comm.hpp"

namespace legio {

class Multicomm
{
   public:
    Multicomm(Multicomm const&) = delete;
    Multicomm& operator=(Multicomm const&) = delete;
    static Multicomm& get_instance()
    {
        static Multicomm instance;
        return instance;
    }

    int add_comm(MPI_Comm);
    ComplexComm& translate_into_complex(MPI_Comm);
    void remove(MPI_Comm, std::function<int(MPI_Comm*)>);
    const bool part_of(const MPI_Comm) const;
    void initialize(const int size);
    void initialize(const int size, const int rank, const std::vector<int> failed);
    const bool is_initialized();

    template <class MPI_T>
    bool add_structure(ComplexComm& comm,
                       MPI_T elem,
                       const std::function<int(MPI_Comm, MPI_T*)> func)
    {
        assert(initialized);
        int id = c2f<MPI_Comm>(comm.get_alias());
        auto res = maps[handle_selector<MPI_T>::get()].insert({c2f<MPI_T>(elem), id});
        if (res.second)
            comm.add_structure(elem, func);
        return res.second;
    }

    void remove_structure(MPI_Win*);
    void remove_structure(MPI_File*);
    void remove_structure(MPI_Request*);

    template <class MPI_T>
    ComplexComm& get_complex_from_structure(MPI_T elem)
    {
        assert(initialized);
        auto res = maps[handle_selector<MPI_T>::get()].find(c2f<MPI_T>(elem));
        if (res != maps[handle_selector<MPI_T>::get()].end())
        {
            // std::unordered_map<int, int>::iterator res2 = comms_order.find(res->second);
            auto res2 = comms.find(res->second);
            if (res2 != comms.end())
                return res2->second;
            else
            {
                assert(false && "COMMUNICATOR NO MORE IN LIST!!!\n");
            }
        }
        else
        {
            assert(false && "STRUCTURE NOT MAPPED\n");
        }
    }

    template <class MPI_T>
    const bool part_of(MPI_T elem) const
    {
        assert(initialized);
        auto result = maps[handle_selector<MPI_T>::get()].find(c2f<MPI_T>(elem));
        return result != maps[handle_selector<MPI_T>::get()].end();
    }

    // void change_comm(ComplexComm &, MPI_Comm);
    std::vector<SupportedComm> supported_comms_vector;
    std::vector<Rank> get_ranks();
    void set_failed_rank(int world_rank);
    int translate_ranks(int, ComplexComm&);
    int untranslate_world_rank(int translated);

    const std::map<int, RespawnedSupportedComm>& access_supported_comms_respawned()
    {
        assert(initialized);
        assert(respawned);
        return supported_comms_respawned;
    }

    const std::vector<int> get_respawn_list() const
    {
        assert(initialized);
        return to_respawn;
    }

    void add_to_respawn_list(const int rank) { to_respawn.push_back(rank); }

    const int& get_own_rank() const
    {
        assert(initialized);
        assert(respawned);
        return own_rank;
    }

    const bool is_respawned() const
    {
        assert(initialized);
        return respawned;
    }

    void add_to_supported_comms(std::pair<int, int> addee)
    {
        assert(initialized);
        supported_comms.insert(addee);
    }

    MPI_Comm get_world_comm();

    void set_world_comm(MPI_Comm);

   private:
    std::mutex init_lock;
    std::mutex world_lock;
    std::vector<Rank> ranks;
    std::unordered_map<int, ComplexComm> comms;
    std::array<std::unordered_map<int, int>, 3> maps;
    std::map<int, RespawnedSupportedComm> supported_comms_respawned;
    bool respawned = false;
    bool initialized = false;
    Multicomm() = default;
    int own_rank;
    std::vector<int> to_respawn;
    std::map<int, int> supported_comms;
    MPI_Comm world_comm = MPI_COMM_NULL;
};

}  // namespace legio

#endif