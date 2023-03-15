#include <mpi.h>
#include <signal.h>
#include <stdio.h>
#include <shared_mutex>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "context.hpp"
#include "log.hpp"
#include "mpi-ext.h"

extern std::shared_timed_mutex failure_mtx;
using namespace legio;

int MPI_Win_create(void* base,
                   MPI_Aint size,
                   int disp_unit,
                   MPI_Info info,
                   MPI_Comm comm,
                   MPI_Win* win)
{
    while (1)
    {
        int rc;
        bool flag = Context::get().m_comm.part_of(comm);
        std::function<int(MPI_Comm, MPI_Win*)> func;
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
            MPI_Barrier(translated.get_alias());
            func = [base, size, disp_unit, info](MPI_Comm c, MPI_Win* w) -> int {
                int rc = PMPI_Win_create(base, size, disp_unit, info, c, w);
                MPI_Win_set_errhandler(*w, MPI_ERRORS_RETURN);
                return rc;
            };
            rc = func(translated.get_comm(), win);
        }
        else
            rc = PMPI_Win_create(base, size, disp_unit, info, comm, win);
        legio::report_execution(rc, comm, "Win_create");
        if (!flag)
            return rc;
        else if (rc == MPI_SUCCESS)
        {
            bool result = Context::get().m_comm.add_structure(
                Context::get().m_comm.translate_into_complex(comm), *win, func);
            if (result)
                return rc;
        }
        else
            replace_comm(Context::get().m_comm.translate_into_complex(comm));
    }
}

int MPI_Win_allocate(MPI_Aint size,
                     int disp_unit,
                     MPI_Info info,
                     MPI_Comm comm,
                     void* baseptr,
                     MPI_Win* win)
{
    while (1)
    {
        int rc;
        std::function<int(MPI_Comm, MPI_Win*)> func;
        bool flag = Context::get().m_comm.part_of(comm);
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
            MPI_Barrier(translated.get_alias());
            func = [size, disp_unit, info, baseptr](MPI_Comm c, MPI_Win* w) -> int {
                int rc = PMPI_Win_allocate(size, disp_unit, info, c, baseptr, w);
                MPI_Win_set_errhandler(*w, MPI_ERRORS_RETURN);
                return rc;
            };
            rc = func(translated.get_comm(), win);
        }
        else
            rc = PMPI_Win_allocate(size, disp_unit, info, comm, baseptr, win);
        legio::report_execution(rc, comm, "Win_allocate");
        if (!flag)
            return rc;
        else if (rc == MPI_SUCCESS)
        {
            bool result = Context::get().m_comm.add_structure(
                Context::get().m_comm.translate_into_complex(comm), *win, func);
            if (result)
                return rc;
        }
        else
            replace_comm(Context::get().m_comm.translate_into_complex(comm));
    }
}

int MPI_Win_free(MPI_Win* win)
{
    Context::get().m_comm.remove_structure(win);
    return MPI_SUCCESS;
}

int MPI_Win_fence(int assert, MPI_Win win)
{
    while (1)
    {
        int rc;
        bool flag = Context::get().m_comm.part_of(win);
        if (flag)
        {
            ComplexComm& comm = Context::get().m_comm.get_complex_from_structure(win);
            MPI_Win translated = comm.translate_structure(win);
            MPI_Barrier(comm.get_alias());
            rc = PMPI_Win_fence(assert, translated);
        }
        else
            rc = PMPI_Win_fence(assert, win);
        legio::report_execution(rc, MPI_COMM_WORLD, "Win_fence");
        if (rc == MPI_SUCCESS || !flag)
            return rc;
        else
            replace_comm(Context::get().m_comm.get_complex_from_structure(win));
    }
}

int MPI_Get(void* origin_addr,
            int origin_count,
            MPI_Datatype origin_datatype,
            int target_rank,
            MPI_Aint target_disp,
            int target_count,
            MPI_Datatype target_datatype,
            MPI_Win win)
{
    int rc;
    bool flag = Context::get().m_comm.part_of(win);

    if (flag)
    {
        ComplexComm& comm = Context::get().m_comm.get_complex_from_structure(win);
        MPI_Win translated = comm.translate_structure(win);
        int new_rank = translate_ranks(target_rank, comm);
        if (new_rank == MPI_UNDEFINED)
        {
            if constexpr (BuildOptions::get_resiliency)
                rc = MPI_SUCCESS;
            else
            {
                legio::log("##### Get failed, stopping a node", LogLevel::errors_only);
                raise(SIGINT);
            }
        }
        else
            rc = PMPI_Get(origin_addr, origin_count, origin_datatype, new_rank, target_disp,
                          target_count, target_datatype, translated);
    }
    else
        rc = PMPI_Get(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                      target_count, target_datatype, win);
    legio::report_execution(rc, MPI_COMM_WORLD, "Get");
    return rc;
}

int MPI_Put(const void* origin_addr,
            int origin_count,
            MPI_Datatype origin_datatype,
            int target_rank,
            MPI_Aint target_disp,
            int target_count,
            MPI_Datatype target_datatype,
            MPI_Win win)
{
    int rc;
    bool flag = Context::get().m_comm.part_of(win);
    if (flag)
    {
        ComplexComm& comm = Context::get().m_comm.get_complex_from_structure(win);
        MPI_Win translated = comm.translate_structure(win);
        int new_rank = translate_ranks(target_rank, comm);
        if (new_rank == MPI_UNDEFINED)
        {
            if constexpr (BuildOptions::put_resiliency)
                rc = MPI_SUCCESS;
            else
            {
                legio::log("##### Put failed, stopping a node", LogLevel::errors_only);
                raise(SIGINT);
            }
        }
        else
            rc = PMPI_Put(origin_addr, origin_count, origin_datatype, new_rank, target_disp,
                          target_count, target_datatype, translated);
    }
    else
        rc = PMPI_Put(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                      target_count, target_datatype, win);
    legio::report_execution(rc, MPI_COMM_WORLD, "Put");
    return rc;
}