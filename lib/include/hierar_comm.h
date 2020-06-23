#ifndef HIERAR_COMM_H
#define HIERAR_COMM_H

#include "mpi.h"
#include "adv_comm.h"
#include <unordered_map>
#include <functional>
#include "structure_handler.h"
#include "operations.h"
#include "comm_manipulation.h"

#define DIMENSION 5     //MOVE ME

class HierarComm : public AdvComm
{
    public:
        HierarComm(MPI_Comm);

        void fault_manage(MPI_Comm);
        void fault_manage(MPI_File);
        void fault_manage(MPI_Win);
        void result_agreement(int*, MPI_Comm);
        void result_agreement(int*, MPI_File);
        void result_agreement(int*, MPI_Win);

        void destroy(std::function<int(MPI_Comm*)>);

        inline bool add_comm(MPI_Comm comm)
        {
            return ::add_comm(comm, this);
        }

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
        int perform_operation(WinOp, int, MPI_Win) {return MPI_SUCCESS;}
        int perform_operation(WinOpColl, MPI_Win) {return MPI_SUCCESS;}
        int perform_operation(LocalOnly);
        int perform_operation(CommCreator);

    private:
        void local_replace_comm(MPI_Comm);
        StructureHandler<MPI_File, MPI_Comm> * files;
        MPI_Comm local;
        MPI_Comm global;
        MPI_Comm full_network;
        int translate_ranks(int old_rank, MPI_Comm new_comm)
        {
            int source = old_rank, tr_rank;
            MPI_Group tr_group;
            MPI_Comm_group(new_comm, &tr_group);
            MPI_Group_translate_ranks(get_group(), 1, &source, tr_group, &tr_rank);
            return tr_rank;
        }
        int extract_group(int rank)
        {
            return rank / DIMENSION;
        }
};

#endif