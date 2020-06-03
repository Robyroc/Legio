#ifndef COMPLEX_COMM_H
#define COMPLEX_COMM_H

#include "mpi.h"
#include "adv_comm.h"
#include <unordered_map>
#include <functional>
#include "structure_handler.h"

class ComplexComm : public AdvComm
{
    public:
        ComplexComm(MPI_Comm);

        void fault_manage();

        inline bool file_support() { return true; }
        inline bool window_support() { return true; }
        void add_structure(MPI_Win, std::function<int(MPI_Comm, MPI_Win *)>);
        void add_structure(MPI_File, std::function<int(MPI_Comm, MPI_File *)>);
        void remove_structure(MPI_Win);
        void remove_structure(MPI_File);
        MPI_Win translate_structure(MPI_Win);
        MPI_File translate_structure(MPI_File);
        void check_served(MPI_Win, int*);
        void check_served(MPI_File, int*);

    private:
        void replace_comm(MPI_Comm);
        StructureHandler<MPI_Win, MPI_Comm> * windows;
        StructureHandler<MPI_File, MPI_Comm> * files;
};

#endif