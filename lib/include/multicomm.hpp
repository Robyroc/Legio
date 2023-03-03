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
#include "mpi_structs.hpp"
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

    int add_comm(Legio_comm);
    ComplexComm& translate_into_complex(Legio_comm);
    void remove(Legio_comm, std::function<int(Legio_comm*)>);
    const bool part_of(const Legio_comm) const;
    void initialize(const int size);
    void initialize(const int size, const int rank, const std::vector<int> failed);
    const bool is_initialized();

    template <class Legio_T>
    bool add_structure(ComplexComm& comm,
                       Legio_T elem,
                       const std::function<int(Legio_comm, Legio_T*)> func)
    {
        assert(initialized);
        int id;
        id = c2f<Legio_comm>(static_cast<Legio_comm>(comm.get_alias()));
        // id = c2f<MPI_Comm>(comm.get_alias());
        auto res = maps[handle_selector<Legio_T>::get()].insert({c2f<Legio_T>(elem), id});
        if (res.second)
            comm.add_structure(elem, func);
        return res.second;
    }

    /*
        void remove_structure(MPI_Win*);
        void remove_structure(MPI_File*);
        void remove_structure(MPI_Request*);
    */
    void remove_structure(Legio_win*);
    void remove_structure(Legio_file*);
    void remove_structure(Legio_request*);

    template <class Legio_T>
    ComplexComm& get_complex_from_structure(Legio_T elem)
    {
        assert(initialized);
        auto res = maps[handle_selector<Legio_T>::get()].find(c2f<Legio_T>(elem));
        if (res != maps[handle_selector<Legio_T>::get()].end())
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

    template <class Legio_T>
    const bool part_of(Legio_T elem) const
    {
        assert(initialized);
        auto result = maps[handle_selector<Legio_T>::get()].find(c2f<Legio_T>(elem));
        return result != maps[handle_selector<Legio_T>::get()].end();
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

    // bool is_horizon_present();

    MPI_Comm get_horizon_comm(MPI_Group);

    void add_horizon_comm(MPI_Comm);

    void add_pending_session(MPI_Session session)
    {
        std::unique_lock<std::mutex> lock(session_lock);
        pending.push_back(session);
    }
    void add_open_session()
    {
        std::unique_lock<std::mutex> lock(session_lock);
        open_sessions++;
    }
    void close_session()
    {
        std::unique_lock<std::mutex> lock(session_lock);
        open_sessions--;
        if (open_sessions == 0)
        {
            for (auto session : pending)
                PMPI_Session_finalize(&session);
        }
    }

   private:
    std::mutex init_lock;
    std::mutex horizon_lock;
    std::mutex session_lock;
    std::vector<MPI_Session> pending;
    int open_sessions = 0;
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
    std::list<MPI_Comm> horizon_comms;
};

}  // namespace legio

#endif