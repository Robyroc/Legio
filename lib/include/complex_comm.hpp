#ifndef COMPLEX_COMM_HPP
#define COMPLEX_COMM_HPP

#include <functional>
#include <list>
#include <unordered_map>
#include "mpi.h"
#include "struct_selector.hpp"
#include "structure_handler.hpp"

namespace legio {

struct FullWindow
{
    int id;
    void* base;
    MPI_Aint size;
    int disp_unit;
    MPI_Info info;
    MPI_Win win;
};

class ComplexComm
{
   public:
    template <class Legio_T>
    inline void add_structure(Legio_T elem, std::function<int(Legio_comm, Legio_T*)> func)
    {
        auto structure_ptr = get_handler<Legio_T>();
        structure_ptr->add(c2f<Legio_T>(elem), elem, func);
    }

    template <class Legio_T>
    inline void remove_structure(Legio_T elem)
    {
        auto structure_ptr = get_handler<Legio_T>();
        structure_ptr->remove(elem);
    }

    template <class Legio_T>
    inline Legio_T translate_structure(const Legio_T elem)
    {
        auto structure_ptr = get_handler<Legio_T>();
        return structure_ptr->translate(elem);
    }

    template <class Legio_T>
    inline bool check_served(Legio_T elem)
    {
        auto structure_ptr = get_handler<Legio_T>();
        return structure_ptr->part_of(elem);
    }

    void replace_comm(MPI_Comm);
    MPI_Comm get_comm();
    ComplexComm(MPI_Comm, int);
    MPI_Group get_group();
    MPI_Comm get_alias();
    int get_alias_id() { return alias_id; }

   private:
    handlers struct_handlers;
    MPI_Comm cur_comm;
    MPI_Group group;
    int alias_id;
    template <class Legio_T>
    inline StructureHandler<Legio_T, MPI_Comm>* get_handler(void)
    {
        return std::get<handle_selector<Legio_T>::get()>(struct_handlers);
    }
};

}  // namespace legio

#endif