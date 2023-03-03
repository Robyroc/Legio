#include <signal.h>
#include <stdio.h>
#include <shared_mutex>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "log.hpp"
#include "mpi.h"
#include ULFM_HDR
#include "multicomm.hpp"

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
        Legio_comm com = comm;
        bool flag = Multicomm::get_instance().part_of(com);
        std::function<int(Legio_comm, Legio_win*)> func;
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(com);
            MPI_Barrier(translated.get_alias());
            func = [base, size, disp_unit, info](Legio_comm c, Legio_win* w) -> int {
                MPI_Win win = *w;
                int rc = PMPI_Win_create(base, size, disp_unit, info, c, &win);
                MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);
                *w = win;
                return rc;
            };
            rc = PMPI_Win_create(base, size, disp_unit, info, translated.get_comm(), win);
            MPI_Win_set_errhandler(*win, MPI_ERRORS_RETURN);
        }
        else
            rc = PMPI_Win_create(base, size, disp_unit, info, comm, win);
        legio::report_execution(rc, comm, "Win_create");
        if (!flag)
            return rc;
        else if (rc == MPI_SUCCESS)
        {
            Legio_win w = *win;
            bool result = Multicomm::get_instance().add_structure(
                Multicomm::get_instance().translate_into_complex(com), w, func);
            if (result)
                return rc;
        }
        else
            replace_comm(Multicomm::get_instance().translate_into_complex(com));
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
        Legio_comm com = comm;
        std::function<int(Legio_comm, Legio_win*)> func;
        bool flag = Multicomm::get_instance().part_of(com);
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(com);
            MPI_Barrier(translated.get_alias());
            func = [size, disp_unit, info, baseptr](Legio_comm c, Legio_win* w) -> int {
                MPI_Win win = *w;
                int rc = PMPI_Win_allocate(size, disp_unit, info, c, baseptr, &win);
                MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);
                *w = win;
                return rc;
            };
            Legio_win w = *win;
            rc = func(translated.get_comm(), &w);
            *win = w;
        }
        else
            rc = PMPI_Win_allocate(size, disp_unit, info, comm, baseptr, win);
        legio::report_execution(rc, comm, "Win_allocate");
        if (!flag)
            return rc;
        else if (rc == MPI_SUCCESS)
        {
            Legio_win w = *win;
            bool result = Multicomm::get_instance().add_structure(
                Multicomm::get_instance().translate_into_complex(com), w, func);
            if (result)
                return rc;
        }
        else
            replace_comm(Multicomm::get_instance().translate_into_complex(comm));
    }
}

int MPI_Win_free(MPI_Win* win)
{
    Legio_win w = *win;
    Multicomm::get_instance().remove_structure(&w);
    *win = w;
    return MPI_SUCCESS;
}

int MPI_Win_fence(int assert, MPI_Win win)
{
    while (1)
    {
        int rc;
        Legio_win w = win;
        bool flag = Multicomm::get_instance().part_of(w);
        if (flag)
        {
            ComplexComm& comm = Multicomm::get_instance().get_complex_from_structure(w);
            MPI_Win translated = comm.translate_structure(w);
            MPI_Barrier(comm.get_alias());
            rc = PMPI_Win_fence(assert, translated);
        }
        else
            rc = PMPI_Win_fence(assert, win);
        legio::report_execution(rc, MPI_COMM_WORLD, "Win_fence");
        if (rc == MPI_SUCCESS || !flag)
            return rc;
        else
            replace_comm(Multicomm::get_instance().get_complex_from_structure(w));
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
    Legio_win w = win;
    bool flag = Multicomm::get_instance().part_of(w);

    if (flag)
    {
        ComplexComm& comm = Multicomm::get_instance().get_complex_from_structure(w);
        MPI_Win translated = comm.translate_structure(w);
        int new_rank = Multicomm::get_instance().translate_ranks(target_rank, comm);
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
    Legio_win w = win;
    bool flag = Multicomm::get_instance().part_of(w);
    if (flag)
    {
        ComplexComm& comm = Multicomm::get_instance().get_complex_from_structure(w);
        MPI_Win translated = comm.translate_structure(w);
        int new_rank = Multicomm::get_instance().translate_ranks(target_rank, comm);
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