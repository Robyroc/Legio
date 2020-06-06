#ifndef ADV_COMM_H
#define ADV_COMM_H

#include "mpi.h"
#include <functional>
#include "operations.h"

class AdvComm
{
    public:
        AdvComm(MPI_Comm comm)
        {
            alias_id = MPI_Comm_c2f(comm);
            PMPI_Comm_dup(comm, &cur_comm);
            MPI_Comm_set_errhandler(cur_comm, MPI_ERRORS_RETURN);
            MPI_Comm_group(comm, &group);
        }

        inline MPI_Comm get_comm()
        {
            return cur_comm;
        }

        inline MPI_Group get_group()
        {
            return group;
        }

        inline MPI_Comm get_alias()
        {
            return MPI_Comm_f2c(alias_id);
        }

        inline void destroy(std::function<int(MPI_Comm*)> destroyer)
        {
            destroyer(&cur_comm);
        }
        
        virtual void fault_manage() = 0;

        virtual bool file_support() = 0;
        virtual bool window_support() = 0;
        virtual void check_served(MPI_Win, int*) = 0;
        virtual void check_served(MPI_File, int*) = 0;
        virtual void add_structure(MPI_Win, std::function<int(MPI_Comm, MPI_Win *)>) = 0;
        virtual void add_structure(MPI_File, std::function<int(MPI_Comm, MPI_File *)>) = 0;
        virtual void remove_structure(MPI_Win) = 0;
        virtual void remove_structure(MPI_File) = 0;
        virtual MPI_Win translate_structure(MPI_Win) = 0;
        virtual MPI_File translate_structure(MPI_File) = 0;

        virtual int perform_operation(OneToOne, int) = 0;
        virtual int perform_operation(OneToAll, int) = 0;
        virtual int perform_operation(AllToOne, int) = 0;
        virtual int perform_operation(AllToAll) = 0;
        virtual int perform_operation(FileOp, MPI_File) = 0;
        virtual int perform_operation(WinOp, int, MPI_Win) = 0;
        virtual int perform_operation(WinOpColl, MPI_Win) = 0;

    protected:
        MPI_Comm cur_comm;
        int translate_ranks(int old_rank)
        {
            int source = old_rank, tr_rank;
            MPI_Group tr_group;
            MPI_Comm_group(get_comm(), &tr_group);
            MPI_Group_translate_ranks(group, 1, &source, tr_group, &tr_rank);
            return tr_rank;
        }

    private:
        MPI_Group group;
        int alias_id;
};

#endif