#ifndef COMPLEX_COMM_H
#define COMPLEX_COMM_H

#include "mpi.h"
#include <list>
#include <unordered_map>
#include <functional>
#include "structure_handler.h"
#include "struct_selector.h"

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
        template<class MPI_T>
        inline void add_structure(MPI_T elem, std::function<int(MPI_Comm, MPI_T *)> func)
        {
            auto structure_ptr = get_handler<MPI_T>();
            structure_ptr->add(c2f<MPI_T>(elem), elem, func);
        }

        template<class MPI_T>
        inline void remove_structure(MPI_T elem)
        {
            auto structure_ptr = get_handler<MPI_T>();
            structure_ptr->remove(elem);
        }

        template<class MPI_T>
        inline MPI_T translate_structure(MPI_T elem)
        {
            auto structure_ptr = get_handler<MPI_T>();
            return structure_ptr->translate(elem);
        }

        template<class MPI_T>
        inline void check_served(MPI_T elem, int* result)
        {
            auto structure_ptr = get_handler<MPI_T>();
            structure_ptr->part_of(elem, result);
        }

        void replace_comm(MPI_Comm);
        MPI_Comm get_comm();
        ComplexComm(MPI_Comm, int);
        MPI_Group get_group();
        MPI_Comm get_alias();
        int get_alias_id() {return alias_id;}

    private:
        handlers struct_handlers;
        MPI_Comm cur_comm;
        MPI_Group group;
        int alias_id;
        template<class MPI_T>
        inline StructureHandler<MPI_T, MPI_Comm> * get_handler(void){
            return std::get<handle_selector<MPI_T>::get()>(struct_handlers);
        }
};

#endif