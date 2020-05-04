#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "complex_comm.h"

extern ComplexComm *cur_complex;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

int MPI_Win_create(void* base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    while(1)
    {
        int rc;
        std::function<int(MPI_Comm, MPI_Win *)> func;
        if(comm == MPI_COMM_WORLD)
        {
            MPI_Barrier(MPI_COMM_WORLD);
            func = [base, size, disp_unit, info] (MPI_Comm c, MPI_Win* w) -> int
            {
                int rc = PMPI_Win_create(base, size, disp_unit, info, c, w);
                MPI_Win_set_errhandler(*w, MPI_ERRORS_RETURN);
                return rc;
            };
            rc = func(cur_complex->get_comm(), win);
        }
        else
            rc = PMPI_Win_create(base, size, disp_unit, info, comm, win);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: win created (error: %s)\n", rank, size, errstr);
        }
        if(comm != MPI_COMM_WORLD)
            return rc;
        else if(rc == MPI_SUCCESS)
        {
            cur_complex->add_structure(*win, func);
            return rc;
        }
        else
            replace_comm(cur_complex);
    }
}

int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win)
{
    while(1)
    {
        int rc;
        std::function<int(MPI_Comm, MPI_Win *)> func;
        if(comm == MPI_COMM_WORLD)
        {
            MPI_Barrier(MPI_COMM_WORLD);
            func = [size, disp_unit, info, baseptr] (MPI_Comm c, MPI_Win* w) -> int
            {
                int rc = PMPI_Win_allocate(size, disp_unit, info, c, baseptr, w);
                MPI_Win_set_errhandler(*w, MPI_ERRORS_RETURN);
                return rc;
            };
            rc = func(cur_complex->get_comm(), win);
        }
        else
            rc = PMPI_Win_allocate(size, disp_unit, info, comm, baseptr, win);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: win allocated (error: %s)\n", rank, size, errstr);
        }
        if(comm != MPI_COMM_WORLD)
            return rc;
        else if(rc == MPI_SUCCESS)
        {
            cur_complex->add_structure(*win, func);
            return rc;
        }
        else
            replace_comm(cur_complex);
    }
}

int MPI_Win_free(MPI_Win *win)
{
    int flag;
    cur_complex->check_global(*win, &flag);
    if(flag)
        cur_complex->remove_structure(*win);
    return MPI_SUCCESS;
}

int MPI_Win_fence(int assert, MPI_Win win)
{
    while(1)
    {
        int rc, flag;
        cur_complex->check_global(win, &flag);
        MPI_Win translated = cur_complex->translate_structure(win);
        if(flag)
            MPI_Barrier(MPI_COMM_WORLD);
        rc = PMPI_Win_fence(assert, translated);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: fence done (error: %s)\n", rank, size, errstr);
        }
        if(rc == MPI_SUCCESS || !flag)
            return rc;
        else
            replace_comm(cur_complex);
    }
}

int MPI_Get(void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
    int rc, flag;
    cur_complex->check_global(win, &flag);
    if(flag)
    {
        MPI_Win translated = cur_complex->translate_structure(win);
        int new_rank;
        translate_ranks(target_rank, cur_complex->get_comm(), &new_rank);
        if(new_rank == MPI_UNDEFINED)
        {
            HANDLE_GET_FAIL(cur_complex->get_comm());
        }
        rc = PMPI_Get(origin_addr, origin_count, origin_datatype, new_rank, target_disp, target_count, target_datatype, translated);
    }
    else
        rc = PMPI_Get(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
    get_handling:
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: get done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_Put(const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
    int rc, flag;
    cur_complex->check_global(win, &flag);
    if(flag)
    {
        MPI_Win translated = cur_complex->translate_structure(win);
        int new_rank;
        translate_ranks(target_rank, cur_complex->get_comm(), &new_rank);
        if(new_rank == MPI_UNDEFINED)
        {
            int rank;
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            HANDLE_PUT_FAIL(cur_complex->get_comm());
        }
        rc = PMPI_Put(origin_addr, origin_count, origin_datatype, new_rank, target_disp, target_count, target_datatype, translated);
    }
    else
        rc = PMPI_Put(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
    put_handling:
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: put done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}