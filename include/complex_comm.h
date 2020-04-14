#ifndef COMPLEX_COMM_H
#define COMPLEX_COMM_H

#include "mpi.h"
#include <list>
#include <unordered_map>

#define MPI_WIN_GLOB 75

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
        void add_window(void*, MPI_Aint, int, MPI_Info, MPI_Win);
        void remove_window(MPI_Win);
        void replace_comm(MPI_Comm);
        MPI_Comm get_comm();
        MPI_Win translate_win(MPI_Win);
        void check_global(MPI_Win, int*);
        ComplexComm(MPI_Comm);

    private:
        MPI_Comm cur_comm;
        std::unordered_map<int, FullWindow> opened_windows;
        int counter;
};

#endif