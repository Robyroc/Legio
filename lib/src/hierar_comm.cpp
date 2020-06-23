#include "hierar_comm.h"
#include "mpi.h"
#include <functional>
#include "mpi-ext.h"

HierarComm::HierarComm(MPI_Comm comm): AdvComm(comm) 
{
    int rank;
    MPI_Comm_rank(comm, &rank);

    int group = extract_group(rank);

    int new_rank = rank%DIMENSION;

    PMPI_Comm_dup(comm, &full_network);
    MPI_Comm_set_errhandler(full_network, MPI_ERRORS_RETURN);
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

void HierarComm::fault_manage(MPI_Comm problematic) 
{
    if(MPI_Comm_c2f(problematic) == MPI_Comm_c2f(local))
    {
        MPI_Comm new_comm;
        int old_size, new_size, diff, old_rank, new_rank;
        MPIX_Comm_shrink(local, &new_comm);
        MPI_Comm_size(local, &old_size);
        MPI_Comm_size(new_comm, &new_size);
        MPI_Comm_rank(local, &old_rank);
        MPI_Comm_rank(new_comm, &new_rank);
        diff = old_size - new_size; /* number of deads */
        if(0 == diff)
            PMPI_Comm_free(&new_comm);
        else
        {
            MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
            if(old_rank != 0 && new_rank == 0)
            {
                // I must join the global group
                MPI_Comm iglobal;   //global intercommunicator
                int rank, rank_group;
                MPI_Comm_rank(get_alias(), &rank);
                rank_group = extract_group(rank);

                //First we must find the rank of a process within global.
                //We are sure that the process with the lowest rank alive
                //will be in global, since its local rank will be 0.
                //We must also check that such process is not part of this group
                //since noone in local has a reference to global (master is dead...)

                int total_size, rc = !MPI_SUCCESS, i;
                MPI_Comm_size(get_alias(), &total_size);
                for(i = 0; i < total_size && rc != MPI_SUCCESS; i++)
                {
                    if(extract_group(i) != rank_group && translate_ranks(i, full_network) != MPI_UNDEFINED)
                        rc = MPI_Intercomm_create(MPI_COMM_SELF, 0, get_alias(), i, 56, &iglobal);
                }
                if(i == total_size)
                {
                    PMPI_Comm_free(&global);
                    //found nothing, all other local groups are dead
                    MPI_Comm_dup(MPI_COMM_SELF, &global);
                }
                else
                {
                    PMPI_Comm_free(&global);
                    MPI_Comm temp_intracomm;
                    PMPI_Intercomm_merge(iglobal, 0, &temp_intracomm);
                    //Here a intracommunicator exists, but the ranks are shuffled

                    PMPI_Comm_split(temp_intracomm, 1, rank, &global);

                    PMPI_Comm_free(&temp_intracomm);
                    PMPI_Comm_free(&iglobal);
                }
            }
            local_replace_comm(new_comm);
        }
    }
    else if(MPI_Comm_c2f(problematic) == MPI_Comm_c2f(full_network))
    {
        //do nothing, failure handled on demand
    }
    else if(MPI_Comm_c2f(problematic) == MPI_Comm_c2f(global))
    {
        MPI_Comm new_comm;
        int old_size, new_size, diff, old_rank, new_rank, global_rank;
        MPIX_Comm_shrink(global, &new_comm);
        MPI_Comm_size(global, &old_size);
        MPI_Comm_size(new_comm, &new_size);
        MPI_Comm_rank(global, &old_rank);
        MPI_Comm_rank(new_comm, &new_rank);
        MPI_Comm_rank(get_alias(), &global_rank);
        int rank_group = extract_group(global_rank);
        diff = old_size - new_size; /* number of deads */
        if(0 == diff)
            PMPI_Comm_free(&new_comm);
        else
        {
            MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);

            //Handle the entrance of the new master, then split to reorder ranks
            MPI_Group old_group, new_group, diff_group;
            MPI_Comm_group(global, &old_group);
            MPI_Comm_group(new_comm, &new_group);
            MPI_Group_difference(old_group, new_group, &diff_group);
            int size_group;
            MPI_Group_size(diff_group, &size_group);
            if(diff != size_group)
                printf("HEY!!! THIS ERROR IS DUE TO GROUP, CHECK HIERARCOMM\n");
            
            int failee, failee_group;
            MPI_Group_translate_ranks(diff_group, 1, 0, get_group(), &failee);
            failee_group = extract_group(failee);
            int i, rc = !MPI_SUCCESS;
            MPI_Comm intercomm;

            for(i = failee_group * DIMENSION; i < (failee_group + 1) * DIMENSION && rc != MPI_SUCCESS; i++)
            {
                if(translate_ranks(i, full_network) != MPI_UNDEFINED)
                    rc = MPI_Intercomm_create(new_comm, 0, get_alias(), i, 56, &intercomm);
            }
            if(i == (failee_group + 1) * DIMENSION)
            {
                //The local rank is completely failed, drop it
                PMPI_Comm_free(&global);
                global = new_comm;
            }
            else
            {
                PMPI_Comm_free(&global);
                MPI_Comm temp_intracomm;
                PMPI_Intercomm_merge(intercomm, 0, &temp_intracomm);
                //Here a intracommunicator exists, but the ranks are shuffled

                PMPI_Comm_split(temp_intracomm, 1, global_rank, &global);

                PMPI_Comm_free(&temp_intracomm);
                PMPI_Comm_free(&intercomm);
            }
        }
    }
}

void HierarComm::fault_manage(MPI_File problematic) 
{
    return fault_manage(local);
}

void HierarComm::fault_manage(MPI_Win problematic) {}

void HierarComm::result_agreement(int* rc, MPI_Comm problematic) 
{
    int flag = (MPI_SUCCESS==*rc);
    int* pointer = &flag;
    
    MPIX_Comm_agree(problematic, pointer);

    if(!flag && *rc == MPI_SUCCESS)
        *rc = MPIX_ERR_PROC_FAILED;
    if(*rc != MPI_SUCCESS)
        fault_manage(problematic);
}

void HierarComm::result_agreement(int* rc, MPI_File problematic) 
{
    return result_agreement(rc, local);
}

void HierarComm::result_agreement(int* rc, MPI_Win problematic) {}

void HierarComm::destroy(std::function<int(MPI_Comm*)> destroyer)
{
    int local_rank;
    MPI_Comm_rank(local, &local_rank);
    destroyer(&local);
    if(local_rank == 0)
        destroyer(&global);
    delete files;
}

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

int HierarComm::perform_operation(OneToOne op, int rank)
{
    int rc = MPI_SUCCESS;
    MPI_Comm intercomm;
    rc = PMPI_Intercomm_create(MPI_COMM_SELF, 0, get_alias(), rank, 46, &intercomm);
    if(rc != MPI_SUCCESS)
        return op(MPI_UNDEFINED, get_alias(), this);
    else
    {
        int self_rank, intracomm_rank;
        MPI_Comm_rank(get_alias(), &self_rank);
        MPI_Comm intracomm;
        PMPI_Intercomm_merge(intercomm, self_rank > rank, &intracomm);
        MPI_Comm_rank(intracomm, &intracomm_rank);
        return op(!intracomm_rank, intracomm, this);
    }
}

int HierarComm::perform_operation(OneToAll op, int rank) 
{
    int root_group = extract_group(rank), this_group, this_rank, this_local_rank;
    int rc;

    MPI_Comm_rank(get_alias(), &this_rank);
    this_group = extract_group(this_rank);

    MPI_Comm_rank(local, &this_local_rank);

    if(root_group == this_group)
    {
        //root is local
        int local_rank = translate_ranks(rank, local);
        rc = op(local_rank, local, this);
        if(this_local_rank == 0)
        {
            //I'm master in this subnet, need to operate on global
            rc = op(translate_ranks(this_rank, global), global, this);
        }
    }
    else
    {
        if(this_local_rank == 0)
        {
            //I'm master in this subnet, need to propagate
            rc = op(translate_ranks(this_rank, global), global, this);
        }
        //root is in another subnet, need to perform using master as root
        rc = op(0, local, this);
    }
    return rc;
}

int HierarComm::perform_operation(AllToOne op, int rank) 
{
    int root_group = extract_group(rank), this_group, this_rank, this_local_rank;
    int rc;

    MPI_Comm_rank(get_alias(), &this_rank);
    this_group = extract_group(this_rank);

    MPI_Comm_rank(local, &this_local_rank);

    if(root_group != this_group)
    {
        //root is not local, aggregating to master
        rc = op(0, local, this);
        if(this_local_rank == 0)
        {
            //I'm master in this subnet, must propagate the results
            rc = op(translate_ranks(this_rank, global), global, this);
        }
    }
    else
    {
        if(this_local_rank == 0)
        {
            //I'm master, need to receive results
            rc = op(translate_ranks(this_rank, global), global, this);
        }
        int local_rank = translate_ranks(rank, local);
        rc = op(local_rank, local, this);
    }
    return rc;
}

int HierarComm::perform_operation(AllToAll op) 
{
    int rc = perform_operation(op.decomp().first, 0);
    rc |= perform_operation(op.decomp().second, 0);
    return rc;
}

int HierarComm::perform_operation(FileOp op, MPI_File file) 
{
    MPI_File translated = translate_structure(file);
    int rc = op(file, this);
}

int HierarComm::perform_operation(LocalOnly op)
{
    return op(local, this);
}

int HierarComm::perform_operation(CommCreator op)
{
    //Shrink on demand, since it's costly
    MPI_Comm new_comm;
    int old_size, new_size, diff;
    MPIX_Comm_shrink(full_network, &new_comm);
    MPI_Comm_size(full_network, &old_size);
    MPI_Comm_size(new_comm, &new_size);
    diff = old_size - new_size; /* number of deads */
    if(0 == diff)
        PMPI_Comm_free(&new_comm);
    else
    {
        MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
        full_network = new_comm;
    }

    return op(full_network, this);
}

void HierarComm::local_replace_comm(MPI_Comm new_comm) 
{
    files->replace(new_comm);
    MPI_Info info;
    PMPI_Comm_get_info(local, &info);
    PMPI_Comm_set_info(new_comm, info);
    PMPI_Info_free(&info);
    PMPI_Comm_free(&local);
    local = new_comm;
}