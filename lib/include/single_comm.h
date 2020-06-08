#ifndef SINGLE_COMM_H
#define SINGLE_COMM_H

#include "mpi.h"
#include "adv_comm.h"
#include <unordered_map>
#include <functional>
#include "structure_handler.h"
#include "operations.h"
#include "comm_manipulation.h"

class SingleComm : public AdvComm
{
    public:
        SingleComm(MPI_Comm);

        void fault_manage();

        inline void destroy(std::function<int(MPI_Comm*)> destroyer)
        {
            destroyer(&cur_comm);
        }

        inline bool add_comm(MPI_Comm comm)
        {
            return ::add_comm(comm, this);
        }

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

        int perform_operation(OneToOne, int);
        int perform_operation(OneToAll, int);
        int perform_operation(AllToOne, int);
        int perform_operation(AllToAll);
        int perform_operation(FileOp, MPI_File);
        int perform_operation(WinOp, int, MPI_Win);
        int perform_operation(WinOpColl, MPI_Win);

    private:
        inline MPI_Comm get_comm() {return cur_comm;};
        MPI_Comm cur_comm;
        void replace_comm(MPI_Comm);
        StructureHandler<MPI_Win, MPI_Comm> * windows;
        StructureHandler<MPI_File, MPI_Comm> * files;
        int translate_ranks(int old_rank)
        {
            int source = old_rank, tr_rank;
            MPI_Group tr_group;
            MPI_Comm_group(get_comm(), &tr_group);
            MPI_Group_translate_ranks(get_group(), 1, &source, tr_group, &tr_rank);
            return tr_rank;
        }
};

#endif