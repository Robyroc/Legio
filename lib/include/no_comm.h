#ifndef NO_COMM_H
#define NO_COMM_H

#include "mpi.h"
#include "adv_comm.h"
#include <unordered_map>
#include <functional>
#include "structure_handler.h"
#include "operations.h"
#include "comm_manipulation.h"

class NoComm : public AdvComm
{
    public:
        NoComm(MPI_Comm comm): AdvComm(comm)                                    {}

        void fault_manage()                                                     {}
        void result_agreement(int*)                                             {}

        void destroy(std::function<int(MPI_Comm*)>)                             {}

        inline bool add_comm(MPI_Comm comm)                                     {return ::add_comm(comm, this);}

        inline bool file_support()                                              {return false;}
        inline bool window_support()                                            {return false;}
        void add_structure(MPI_Win, std::function<int(MPI_Comm, MPI_Win *)>)    {}
        void add_structure(MPI_File, std::function<int(MPI_Comm, MPI_File *)>)  {}
        void remove_structure(MPI_Win)                                          {}
        void remove_structure(MPI_File)                                         {}
        MPI_Win translate_structure(MPI_Win win)                                {return win;}
        MPI_File translate_structure(MPI_File file)                             {return file;};
        void check_served(MPI_Win, int*)                                        {}
        void check_served(MPI_File, int*)                                       {}

        int perform_operation(OneToAll func, int root)                          {return func(root, get_alias(), this);}
        int perform_operation(OneToOne func, int root)                          {return func(root, get_alias(), this);} 
        int perform_operation(AllToOne func, int root)                          {return func(root, get_alias(), this);}
        int perform_operation(AllToAll func)                                    {return func(get_alias(), this);}
        int perform_operation(FileOp func, MPI_File file)                       {return func(file, this);}
        int perform_operation(WinOp func, int root, MPI_Win win)                {return func(root, win, this);}
        int perform_operation(WinOpColl func, MPI_Win win)                      {return func(win, this);}

};

#endif