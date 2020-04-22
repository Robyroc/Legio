#include "complex_comm.h"
#include "mpi.h"
#include "structure_handler.h"

ComplexComm::ComplexComm(MPI_Comm comm):cur_comm(comm)
{
    int keyval;
    MPI_Win_create_keyval(MPI_WIN_NULL_COPY_FN, MPI_WIN_NULL_DELETE_FN, &keyval, (void*)0);
    std::function<int(MPI_Win, int*)> setter_w = [keyval](MPI_Win w, int* value) -> int
    {
        return MPI_Win_set_attr(w, keyval, value);
    };
    std::function<int(MPI_Win, int*, int*)> getter_w = [keyval](MPI_Win w, int* key, int* flag) -> int
    {
        return MPI_Win_get_attr(w, keyval, key, flag);
    };
    std::function<int(MPI_Win*)> killer_w = [](MPI_Win *w) -> int
    {
        return PMPI_Win_free(w);
    };
    windows = new StructureHandler<MPI_Win, MPI_Comm>(setter_w, getter_w, killer_w);

    std::function<int(MPI_File, int*)> setter_f = [] (MPI_File f, int* value) -> int
    {
        MPI_Info info;
        MPI_File_get_info(f, &info);
        MPI_Info_set(info, "legio", (char*) value);
        return MPI_File_set_info(f, info);
    };

    std::function<int(MPI_File, int*, int*)> getter_f = [] (MPI_File f, int* key, int* flag) -> int
    {
        MPI_Info info;
        MPI_File_get_info(f, &info);
        return MPI_Info_get(info, "legio", sizeof(int) / sizeof(char), (char*) key, flag);
    };
    std::function<int(MPI_File*)> killer_f = [](MPI_File *f) -> int
    {
        return PMPI_File_close(f);
    };
    files = new StructureHandler<MPI_File, MPI_Comm>(setter_f, getter_f, killer_f);
}

void ComplexComm::add_structure(MPI_Win win, std::function<int(MPI_Comm, MPI_Win*)> func)
{
    windows->add(win, func);
}

void ComplexComm::add_structure(MPI_File file, std::function<int(MPI_Comm, MPI_File*)> func)
{
    files->add(file, func);
}

MPI_Win ComplexComm::translate_structure(MPI_Win win)
{
    return windows->translate(win);
}

MPI_File ComplexComm::translate_structure(MPI_File file)
{
    return files->translate(file);
}

void ComplexComm::remove_structure(MPI_Win win)
{
    windows->remove(win);
}

void ComplexComm::remove_structure(MPI_File file)
{
    files->remove(file);
}

MPI_Comm ComplexComm::get_comm()
{
    return cur_comm;
}

void ComplexComm::replace_comm(MPI_Comm comm)
{
    windows->replace(comm);
    files->replace(comm);
    PMPI_Comm_free(&cur_comm);
    cur_comm = comm;
}

void ComplexComm::check_global(MPI_Win win, int* result)
{
    windows->part_of(win, result);
}

void ComplexComm::check_global(MPI_File file, int* result)
{
    files->part_of(file, result);
}