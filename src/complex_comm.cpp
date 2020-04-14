#include "complex_comm.h"
#include "mpi.h"

ComplexComm::ComplexComm(MPI_Comm comm):cur_comm(comm), counter(0)
{}

void ComplexComm::add_window(void* base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Win win)
{
    FullWindow window;
    window.id = counter++;
    window.base = base;
    window.size = size;
    window.disp_unit = disp_unit;
    window.info = info;
    window.win = win;
    std::pair<int, FullWindow> inserting (window.id, window);
    opened_windows.insert(inserting);
    std::unordered_map<int, FullWindow>::iterator res = opened_windows.find(window.id);
    MPI_Win_set_attr(win, MPI_WIN_GLOB, &(res->second.id));
    MPI_Win_set_attr(res->second.win, MPI_WIN_GLOB, &(res->second.id));
}

MPI_Win ComplexComm::translate_win(MPI_Win win)
{
    int value;
    int flag;
    MPI_Win_get_attr(win, MPI_WIN_GLOB, &value, &flag);
    if(flag)
    {
        std::unordered_map<int, FullWindow>::const_iterator res = opened_windows.find(value);
        if(res == opened_windows.end())
            return win;
        else return res->second.win;
    }
    else return win;
}

void ComplexComm::remove_window(MPI_Win win)
{
    int key;
    int flag;
    MPI_Win_get_attr(win, MPI_WIN_GLOB, &key, &flag);
    if(flag)
    {
        std::unordered_map<int, FullWindow>::iterator res = opened_windows.find(key);
        if(res != opened_windows.end())
            PMPI_Win_free(&(res->second.win));
        opened_windows.erase(key);
    }
}

MPI_Comm ComplexComm::get_comm()
{
    return cur_comm;
}

void ComplexComm::replace_comm(MPI_Comm comm)
{
    for(auto &w : opened_windows)
        PMPI_Win_free(&(w.second.win));
    PMPI_Comm_free(&cur_comm);
    cur_comm = comm;
    for(auto &w : opened_windows)
    {
        PMPI_Win_create(w.second.base, w.second.size, w.second.disp_unit, w.second.info, comm, &(w.second.win));
        MPI_Win_set_attr(w.second.win, MPI_WIN_GLOB, &(w.second.id));
    }
}

void ComplexComm::check_global(MPI_Win win, int* result)
{
    int value;
    MPI_Win_get_attr(win, MPI_WIN_GLOB, &value, result);
}