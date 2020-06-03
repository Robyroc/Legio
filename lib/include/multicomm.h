#ifndef MULTICOMM_H
#define MULTICOMM_H

#include <unordered_map>
#include "mpi.h"

class AdvComm;

class Multicomm
{
    public:
        template <typename T>
        bool add_comm(MPI_Comm added)
        {
            int id = MPI_Comm_c2f(added);
            AdvComm* temp = new T(added);
            std::pair<int, AdvComm*> adding(id, temp);
            auto res = comms.insert(adding);
            return res.second;
        }

        AdvComm* translate_into_complex(MPI_Comm);
        void remove(MPI_Comm, std::function<int(MPI_Comm*)>);
        void part_of(MPI_Comm, int*);
        Multicomm();
        bool add_file(AdvComm*, MPI_File, std::function<int(MPI_Comm, MPI_File *)>);
        bool add_window(AdvComm*, MPI_Win, std::function<int(MPI_Comm, MPI_Win *)>);
        void remove_window(MPI_Win*);
        void remove_file(MPI_File*);
        AdvComm* get_complex_from_win(MPI_Win);
        AdvComm* get_complex_from_file(MPI_File);
    private:
        std::unordered_map<int, AdvComm*> comms;
        std::unordered_map<int, int> window_map;
        std::unordered_map<int, int> file_map;
};

#endif