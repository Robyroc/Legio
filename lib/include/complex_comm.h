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
        void check_global(MPI_Win, int*);
        void check_global(MPI_File, int*);
        ComplexComm(MPI_Comm);

    private:
        MPI_Comm cur_comm;
        StructureHandler<MPI_Win, MPI_Comm> * windows;
        StructureHandler<MPI_File, MPI_Comm> * files;
};

#endif