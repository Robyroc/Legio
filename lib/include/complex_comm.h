#ifndef COMPLEX_COMM_H
#define COMPLEX_COMM_H

#include "mpi.h"
#include <list>
#include <unordered_map>
#include <functional>
#include "structure_handler.h"

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
        void add_structure(MPI_Win, std::function<int(MPI_Comm, MPI_Win *)>);
        void add_structure(MPI_File, std::function<int(MPI_Comm, MPI_File *)>);
        void remove_structure(MPI_Win);
        void remove_structure(MPI_File);
        void replace_comm(MPI_Comm);
        MPI_Comm get_comm();
        MPI_Win translate_structure(MPI_Win);
        MPI_File translate_structure(MPI_File);
        void check_served(MPI_Win, int*);
        void check_served(MPI_File, int*);
        ComplexComm(MPI_Comm, int, int, std::function<int(MPI_Comm, MPI_Comm*)>, std::function<int(MPI_Comm, MPI_Comm, MPI_Comm*)> = nullptr, int = 0);
        MPI_Group get_group();
        MPI_Comm get_alias();
        void destroy(std::function<int(MPI_Comm*)>);
        int get_parent() {return parent;}
        ComplexComm regenerate(MPI_Comm, MPI_Comm);
        void reapply_destruction();
        int get_parent_id() {return parent;}

    private:
        MPI_Comm cur_comm;
        MPI_Group group;
        StructureHandler<MPI_Win, MPI_Comm> * windows;
        StructureHandler<MPI_File, MPI_Comm> * files;
        int alias_id;
        std::function<int(MPI_Comm, MPI_Comm*)> generator;
        std::function<int(MPI_Comm, MPI_Comm, MPI_Comm*)> inter_generator;
        std::function<int(MPI_Comm*)> destructor;
        int parent;
        int second_parent;
};

#endif