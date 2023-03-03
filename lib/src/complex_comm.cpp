#include "complex_comm.hpp"
#include <mutex>
#include "mpi.h"
#include "request_handler.hpp"
#include "restart.h"
#include "structure_handler.hpp"

extern std::mutex change_world_mtx;
using namespace legio;

ComplexComm::ComplexComm(MPI_Comm comm, int id) : cur_comm(comm), alias_id(id)
{
    std::function<int(Legio_win, int*)> setter_w = [](Legio_win w, int* value) -> int {
        return MPI_SUCCESS;
    };

    std::function<int(Legio_win, int*, int*)> getter_w = [](Legio_win f, int* key,
                                                            int* flag) -> int {
        int* pointer = *((int**)key);
        *flag = 1;
        *pointer = c2f<Legio_win>(f);
        return MPI_SUCCESS;
    };

    std::function<int(Legio_win*)> killer_w = [](Legio_win* w) -> int {
        MPI_Win win = *w;
        return PMPI_Win_free(&win);
    };

    std::function<int(Legio_win, Legio_win*)> adapter_w = [](Legio_win, Legio_win*) -> int {
        return MPI_SUCCESS;
    };

    std::get<handle_selector<Legio_win>::get()>(struct_handlers) =
        new StructureHandler<Legio_win, MPI_Comm>(setter_w, getter_w, killer_w, adapter_w, 0);

    std::function<int(Legio_file, int*)> setter_f = [](Legio_file f, int* value) -> int {
        return MPI_SUCCESS;
    };

    std::function<int(Legio_file, int*, int*)> getter_f = [](Legio_file f, int* key,
                                                             int* flag) -> int {
        int* pointer = *((int**)key);
        *flag = 0;
        *pointer = c2f<Legio_file>(f);
        return MPI_SUCCESS;
    };

    std::function<int(Legio_file*)> killer_f = [](Legio_file* f) -> int {
        MPI_File file = *f;
        return PMPI_File_close(&file);
    };

    std::function<int(Legio_file, Legio_file*)> adapter_f = [](Legio_file old,
                                                               Legio_file* updated) -> int {
        MPI_Offset disp;
        MPI_Datatype etype, filetype;
        char datarep[MPI_MAX_DATAREP_STRING];
        PMPI_File_get_view(old, &disp, &etype, &filetype, datarep);
        PMPI_File_set_view(*updated, disp, etype, filetype, datarep, MPI_INFO_NULL);
        PMPI_File_get_position(old, &disp);
        PMPI_File_seek(*updated, disp, MPI_SEEK_SET);
        PMPI_File_get_position_shared(old, &disp);
        int rc = PMPI_File_seek_shared(*updated, disp, MPI_SEEK_SET);
        return rc;
    };

    std::get<handle_selector<Legio_file>::get()>(struct_handlers) =
        new StructureHandler<Legio_file, MPI_Comm>(setter_f, getter_f, killer_f, adapter_f, 1);

    std::function<int(Legio_request, int*)> setter_r = [](Legio_request r, int* value) -> int {
        return MPI_SUCCESS;
    };

    std::function<int(Legio_request, int*, int*)> getter_r = [](Legio_request r, int* key,
                                                                int* flag) -> int {
        int* pointer = *((int**)key);
        *flag = 0;
        *pointer = c2f<Legio_request>(r);
        return MPI_SUCCESS;
    };

    std::function<int(Legio_request*)> killer_r = [](Legio_request* r) -> int {
        // return PMPI_Request_free(r);
        return MPI_SUCCESS;
    };

    std::function<int(Legio_request, Legio_request*)> adapter_r =
        [](Legio_request old, Legio_request* updated) -> int { return MPI_SUCCESS; };

    std::get<handle_selector<Legio_request>::get()>(struct_handlers) =
        new RequestHandler(setter_r, getter_r, killer_r, adapter_r, 1);

    MPI_Comm_group(comm, &group);
}

MPI_Comm ComplexComm::get_comm()
{
    return cur_comm;
}

void ComplexComm::replace_comm(MPI_Comm comm)
{
    if (get_alias() == MPI_COMM_WORLD)
    {
        change_world_mtx.lock();
    }
    get_handler<Legio_win>()->replace(comm);
    // windows->replace(comm);
    get_handler<Legio_file>()->replace(comm);
    // files->replace(comm);
    get_handler<Legio_request>()->replace(comm);
    // requests->replace(comm);
    MPI_Info info;
    PMPI_Comm_get_info(cur_comm, &info);
    PMPI_Comm_set_info(comm, info);
    PMPI_Info_free(&info);
    PMPI_Comm_free(&cur_comm);
    cur_comm = comm;
    if (get_alias() == MPI_COMM_WORLD)
    {
        change_world_mtx.unlock();
    }
}

MPI_Group ComplexComm::get_group()
{
    return group;
}

MPI_Comm ComplexComm::get_alias()
{
    return MPI_Comm_f2c(alias_id);
}