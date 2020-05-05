#ifndef MULTICOMM_H
#define MULTICOMM_H

#include <unordered_map>
#include "complex_comm.h"
#include "mpi.h"

class Multicomm
{
    public:
        void add_comm(MPI_Comm);
        ComplexComm* translate_into_complex(MPI_Comm);
        void remove(MPI_Comm);
        void part_of(MPI_Comm, int*);
        Multicomm();
        void add_file(ComplexComm*, MPI_File, std::function<int(MPI_Comm, MPI_File *)>);
        void add_window(ComplexComm*, MPI_Win, std::function<int(MPI_Comm, MPI_Win *)>);
        void remove_window(MPI_Win*);
        void remove_file(MPI_File*);
        ComplexComm* get_complex_from_win(MPI_Win);
        ComplexComm* get_complex_from_file(MPI_File);
    private:
        std::unordered_map<int, ComplexComm> comms;
        int counter;
        int keyval;
        std::unordered_map<int, int> window_map;
        std::unordered_map<int, int> file_map;
};

#endif