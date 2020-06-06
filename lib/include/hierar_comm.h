#ifndef HIERAR_COMM_H
#define HIERAR_COMM_H

#include "mpi.h"
#include "adv_comm.h"
#include <unordered_map>
#include <functional>
#include "structure_handler.h"
#include "operations.h"

class HierarComm : public AdvComm
{
    public:
        HierarComm(MPI_Comm);

        void fault_manage();

        inline bool file_support() { return true; }
        inline bool window_support() { return false; }
        void add_structure(MPI_Win, std::function<int(MPI_Comm, MPI_Win *)>) {}
        void add_structure(MPI_File, std::function<int(MPI_Comm, MPI_File *)>);
        void remove_structure(MPI_Win) {}
        void remove_structure(MPI_File);
        MPI_Win translate_structure(MPI_Win w) {return w;}
        MPI_File translate_structure(MPI_File);
        void check_served(MPI_Win, int*) {}
        void check_served(MPI_File, int*);

        int perform_operation(OneToOne, int);
        int perform_operation(OneToAll, int);
        int perform_operation(AllToOne, int);
        int perform_operation(AllToAll);
        int perform_operation(FileOp, MPI_File);
        int perform_operation(FileOpColl, MPI_File);
        int perform_operation(WinOp, int, MPI_Win);
        int perform_operation(WinOpColl, MPI_Win);

    private:
        void replace_comm(MPI_Comm);
        StructureHandler<MPI_File, MPI_Comm> * files;
};

#endif