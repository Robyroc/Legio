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
    Multicomm(Multicomm&&) = default;
    Multicomm& operator=(Multicomm&&) = default;
    Multicomm() = default;

    int add_comm(MPI_Comm);
    ComplexComm& translate_into_complex(MPI_Comm);
    void remove(MPI_Comm, std::function<int(MPI_Comm*)>);
    const bool part_of(const MPI_Comm) const;

    template <class MPI_T>
    bool add_structure(ComplexComm& comm,
                       MPI_T elem,
                       const std::function<int(MPI_Comm, MPI_T*)> func)
    {
        // assert(initialized);
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
        // assert(initialized);
        auto res = maps[handle_selector<MPI_T>::get()].find(c2f<MPI_T>(elem));
        if (res != maps[handle_selector<MPI_T>::get()].end())
        {
            // std::unordered_map<int, int>::iterator res2 = comms_order.find(res->second);
            auto res2 = comms.find(res->second);
            if (res2 != comms.end())
                return res2->second;
            else
                assert(false && "COMMUNICATOR NO MORE IN LIST!!!\n");
        }
        else
            assert(false && "STRUCTURE NOT MAPPED\n");
        exit(0);
    }

    template <class MPI_T>
    const bool part_of(MPI_T elem) const
    {
        // assert(initialized);
        auto result = maps[handle_selector<MPI_T>::get()].find(c2f<MPI_T>(elem));
        return result != maps[handle_selector<MPI_T>::get()].end();
    }

   private:
    std::unordered_map<int, ComplexComm> comms;
    std::array<std::unordered_map<int, int>, 3> maps;
};

}  // namespace legio

#endif