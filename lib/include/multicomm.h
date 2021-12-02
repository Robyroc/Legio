#ifndef MULTICOMM_H
#define MULTICOMM_H

#include <unordered_map>
#include <map>
#include "mpi.h"
#include <functional>

class ComplexComm;

class Multicomm
{
    public:
        int add_comm(MPI_Comm, MPI_Comm, std::function<int(MPI_Comm, MPI_Comm*)>, std::function<int(MPI_Comm, MPI_Comm, MPI_Comm*)> = nullptr, MPI_Comm = MPI_COMM_NULL);
        int add_comm(ComplexComm);
        ComplexComm* translate_into_complex(MPI_Comm);
        void remove(MPI_Comm, std::function<int(MPI_Comm*)>);
        void part_of(MPI_Comm, int*);
        Multicomm();
        bool add_file(ComplexComm*, MPI_File, std::function<int(MPI_Comm, MPI_File *)>);
        bool add_window(ComplexComm*, MPI_Win, std::function<int(MPI_Comm, MPI_Win *)>);
        void remove_window(MPI_Win*);
        void remove_file(MPI_File*);
        ComplexComm* get_complex_from_win(MPI_Win);
        ComplexComm* get_complex_from_file(MPI_File);
        void change_comm(ComplexComm*, MPI_Comm);
    private:
        std::map<int, ComplexComm> comms;
        std::unordered_map<int, int> window_map;
        std::unordered_map<int, int> file_map;
        std::unordered_map<int, int> comms_order;
        int size;
};

#endif