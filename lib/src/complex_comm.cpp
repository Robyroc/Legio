#include "complex_comm.h"
#include "mpi.h"
#include <mutex>
#include "structure_handler.h"
#include "request_handler.h"
#include "restart.h"


extern std::mutex change_world_mtx;


ComplexComm::ComplexComm(MPI_Comm comm, int id)
    :cur_comm(comm),
    alias_id(id)
{    
    std::function<int(MPI_Win, int*)> setter_w = [](MPI_Win w, int* value) -> int {return MPI_SUCCESS;};

    std::function<int(MPI_Win, int*, int*)> getter_w = [] (MPI_Win f, int* key, int* flag) -> int
    {
        int *pointer = *((int**) key);
        *flag = 1;
        *pointer = c2f<MPI_Win>(f);
        return MPI_SUCCESS;
    };

    std::function<int(MPI_Win*)> killer_w = [](MPI_Win *w) -> int
    {
        return PMPI_Win_free(w);
    };

    std::function<int(MPI_Win, MPI_Win*)> adapter_w = [] (MPI_Win, MPI_Win*) -> int {return MPI_SUCCESS;};

    std::get<handle_selector<MPI_Win>::get()>(struct_handlers) = new StructureHandler<MPI_Win, MPI_Comm>(setter_w, getter_w, killer_w, adapter_w, 0);
    
    std::function<int(MPI_File, int*)> setter_f = [] (MPI_File f, int* value) -> int {return MPI_SUCCESS;};

    std::function<int(MPI_File, int*, int*)> getter_f = [] (MPI_File f, int* key, int* flag) -> int
    {
        int *pointer = *((int**) key);
        *flag = 0;
        *pointer = c2f<MPI_File>(f);
        return MPI_SUCCESS;
    };

    std::function<int(MPI_File*)> killer_f = [](MPI_File *f) -> int
    {
        return PMPI_File_close(f);
    };

    std::function<int(MPI_File, MPI_File*)> adapter_f = [] (MPI_File old, MPI_File* updated) -> int
    {
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

    std::get<handle_selector<MPI_File>::get()>(struct_handlers) = new StructureHandler<MPI_File, MPI_Comm>(setter_f, getter_f, killer_f, adapter_f, 1);

    std::function<int(MPI_Request, int*)> setter_r = [] (MPI_Request r, int* value) -> int {return MPI_SUCCESS;};

    std::function<int(MPI_Request, int*, int*)> getter_r = [] (MPI_Request r, int* key, int* flag) -> int
    {
        int *pointer = *((int**) key);
        *flag = 0;
        *pointer = c2f<MPI_Request>(r);
        return MPI_SUCCESS;
    };

    std::function<int(MPI_Request*)> killer_r = [](MPI_Request *r) -> int
    {
        //return PMPI_Request_free(r);
        return MPI_SUCCESS;
    };

    std::function<int(MPI_Request, MPI_Request*)> adapter_r = [] (MPI_Request old, MPI_Request* updated) -> int {return MPI_SUCCESS;};

    std::get<handle_selector<MPI_Request>::get()>(struct_handlers) = new RequestHandler(setter_r, getter_r, killer_r, adapter_r, 1);

    MPI_Comm_group(comm, &group);
}

MPI_Comm ComplexComm::get_comm()
{
    return cur_comm;
}

void ComplexComm::replace_comm(MPI_Comm comm)
{
    if (get_alias() == MPI_COMM_WORLD) {
        change_world_mtx.lock();
    }
    get_handler<MPI_Win>()->replace(comm);
    //windows->replace(comm);
    get_handler<MPI_File>()->replace(comm);
    //files->replace(comm);
    get_handler<MPI_Request>()->replace(comm);
    //requests->replace(comm);
    MPI_Info info;
    PMPI_Comm_get_info(cur_comm, &info);
    PMPI_Comm_set_info(comm, info);
    PMPI_Info_free(&info);
    PMPI_Comm_free(&cur_comm);
    cur_comm = comm;
    if (get_alias() == MPI_COMM_WORLD) {
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