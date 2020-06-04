#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "adv_comm.h"
#include "multicomm.h"

extern Multicomm *cur_comms;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

int MPI_Win_create(void* base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_complex(comm);
        std::function<int(MPI_Comm, MPI_Win *)> func;
        if(flag)
        {
            MPI_Barrier(translated->get_alias());
            func = [base, size, disp_unit, info] (MPI_Comm c, MPI_Win* w) -> int
            {
                int rc = PMPI_Win_create(base, size, disp_unit, info, c, w);
                MPI_Win_set_errhandler(*w, MPI_ERRORS_RETURN);
                return rc;
            };
            rc = func(translated->get_comm(), win);
        }
        else
            rc = PMPI_Win_create(base, size, disp_unit, info, comm, win);
        
        print_info("win_create", comm, rc);

        if(!flag)
            return rc;
        else if(rc == MPI_SUCCESS)
        {
            bool result = cur_comms->add_window(translated, *win, func);
            if(result)
                return rc;
        }
        else
            replace_comm(translated);
    }
}

int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win)
{
    while(1)
    {
        int rc, flag;
        std::function<int(MPI_Comm, MPI_Win *)> func;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_complex(comm);
        if(flag)
        {
            MPI_Barrier(translated->get_alias());
            func = [size, disp_unit, info, baseptr] (MPI_Comm c, MPI_Win* w) -> int
            {
                int rc = PMPI_Win_allocate(size, disp_unit, info, c, baseptr, w);
                MPI_Win_set_errhandler(*w, MPI_ERRORS_RETURN);
                return rc;
            };
            rc = func(translated->get_comm(), win);
        }
        else
            rc = PMPI_Win_allocate(size, disp_unit, info, comm, baseptr, win);
        
        print_info("win_allocate", comm, rc);

        if(!flag)
            return rc;
        else if(rc == MPI_SUCCESS)
        {
            bool result = cur_comms->add_window(translated, *win, func);
            if(result)
                return rc;
        }
        else
            replace_comm(translated);
    }
}

int MPI_Win_free(MPI_Win *win)
{
    cur_comms->remove_window(win);
    return MPI_SUCCESS;
}

int MPI_Win_fence(int assert, MPI_Win win)
{
    while(1)
    {
        int rc;
        AdvComm* comm = cur_comms->get_complex_from_win(win); 
        if(comm != NULL)
        {
            MPI_Win translated = comm->translate_structure(win);
            MPI_Barrier(comm->get_alias());
            rc = PMPI_Win_fence(assert, translated);
        }
        else
            rc = PMPI_Win_fence(assert, win);
        
        print_info("fence", MPI_COMM_WORLD, rc);

        if(rc == MPI_SUCCESS || comm == NULL)
            return rc;
        else
            replace_comm(comm);
    }
}

int MPI_Get(void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
    int rc;
    AdvComm* comm = cur_comms->get_complex_from_win(win); 
    if(comm != NULL)
    {
        MPI_Win translated = comm->translate_structure(win);
        int new_rank;
        translate_ranks(target_rank, comm, &new_rank);
        if(new_rank == MPI_UNDEFINED)
        {
            HANDLE_GET_FAIL(comm->get_comm());
        }
        rc = PMPI_Get(origin_addr, origin_count, origin_datatype, new_rank, target_disp, target_count, target_datatype, translated);
    }
    else
        rc = PMPI_Get(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
    get_handling:
    
    print_info("get", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_Put(const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
    int rc;
    AdvComm* comm = cur_comms->get_complex_from_win(win); 
    if(comm != NULL)
    {
        MPI_Win translated = comm->translate_structure(win);
        int new_rank;
        translate_ranks(target_rank, comm, &new_rank);
        if(new_rank == MPI_UNDEFINED)
        {
            HANDLE_PUT_FAIL(comm->get_comm());
        }
        rc = PMPI_Put(origin_addr, origin_count, origin_datatype, new_rank, target_disp, target_count, target_datatype, translated);
    }
    else
        rc = PMPI_Put(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
    put_handling:
    
    print_info("put", MPI_COMM_WORLD, rc);

    return rc;
}