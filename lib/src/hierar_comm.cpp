#include "hierar_comm.h"
#include "mpi.h"
#include <functional>

#define DIMENSION 5     //MOVE ME

HierarComm::HierarComm(MPI_Comm comm): AdvComm(comm) 
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int group = size/DIMENSION;
    int new_rank = size%DIMENSION;
    PMPI_Comm_split(comm, group, new_rank, &local);
    MPI_Comm_set_errhandler(local, MPI_ERRORS_RETURN);
    PMPI_Comm_split(comm, new_rank == 0, rank, &global);
    if(new_rank != 0)
        MPI_Comm_free(&global);
    else
        MPI_Comm_set_errhandler(global, MPI_ERRORS_RETURN);

    std::function<int(MPI_File, int*)> setter_f = [] (MPI_File f, int* value) -> int {return MPI_SUCCESS;};

    std::function<int(MPI_File, int*, int*)> getter_f = [] (MPI_File f, int* key, int* flag) -> int
    {
        int *pointer = *((int**) key);
        *flag = 0;
        *pointer = MPI_File_c2f(f);
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
    
    files = new StructureHandler<MPI_File, MPI_Comm>(setter_f, getter_f, killer_f, adapter_f, 1);
}

void HierarComm::fault_manage() {}

void HierarComm::result_agreement(int* flag) {}

void HierarComm::destroy(std::function<int(MPI_Comm*)> destroyer) {}

void HierarComm::add_structure(MPI_File file, std::function<int(MPI_Comm, MPI_File*)> func)
{
    files->add(MPI_File_c2f(file), file, func);
}

void HierarComm::remove_structure(MPI_File file) 
{
    files->remove(file);
}

MPI_File HierarComm::translate_structure(MPI_File file) 
{
    return files->translate(file);
}

void HierarComm::check_served(MPI_File file, int* result) 
{
    files->part_of(file, result);
}

int HierarComm::perform_operation(OneToOne op, int rank) {return MPI_SUCCESS;}

int HierarComm::perform_operation(OneToAll op, int rank) {return MPI_SUCCESS;}

int HierarComm::perform_operation(AllToOne op, int rank) {return MPI_SUCCESS;}

int HierarComm::perform_operation(AllToAll op) {return MPI_SUCCESS;}

int HierarComm::perform_operation(FileOp op, MPI_File file) {return MPI_SUCCESS;}

void HierarComm::replace_comm(MPI_Comm new_comm) {}