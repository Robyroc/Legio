#include "hierar_comm.h"
#include "mpi.h"
#include <functional>
#include "mpi-ext.h"
#include <thread>
#include <chrono>
#include <string>

#define PERIOD 5
#define PRINT_DETAILS 1

void print_details(std::string, int);

void HierarComm::change_even_if_unnotified(int rank_group)
{
    int rank;
    MPI_Comm_rank(get_alias(), &rank);
    print_details("Daemon thread booted up...", rank);
    while(1)
    {
        int flag = 0, buf;
        if(local != MPI_COMM_NULL)
            MPI_Iprobe(0, 77, local, &flag, MPI_STATUS_IGNORE);
        if(flag)
        {
            print_details("Found a message!!!", rank);
            PMPI_Recv(&buf, 1, MPI_INT, 0, 77, local, MPI_STATUS_IGNORE);
            MPI_Comm icomm, temp;
            MPIX_Comm_shrink(partially_overlapped_own, &temp);
            PMPI_Comm_free(&partially_overlapped_own);
            print_details("Creating own icomm, token: " + std::to_string(50+rank_group), rank);
            MPI_Intercomm_create(temp, 0, MPI_COMM_NULL, 0, 50 + rank_group, &icomm);
            MPI_Intercomm_merge(icomm, 0, &partially_overlapped_own);
            MPI_Comm_set_errhandler(partially_overlapped_own, MPI_ERRORS_RETURN);
            PMPI_Comm_free(&icomm);
            PMPI_Comm_free(&temp);
            print_details("Daemon done", rank);
        }
        std::this_thread::sleep_for(std::chrono::seconds(PERIOD));
    }
}

HierarComm::HierarComm(MPI_Comm comm): AdvComm(comm) 
{
    int rank;               //Rank in alias
    int group;              //Group of which the process is part
    int local_rank;         //Rank in local
    int size;               //Size of alias

    MPI_Comm_rank(comm, &rank);
    group = extract_group(rank);
    local_rank = rank%DIMENSION;
    MPI_Comm_size(comm, &size);

    if(rank == 0)
        printf("HIERARCOMM LET'S GO!\n");

    //Construction of the needed communicators

    PMPI_Comm_dup(comm, &full_network);                         //Creation of full_network           
    MPI_Comm_set_errhandler(full_network, MPI_ERRORS_RETURN);

    PMPI_Comm_split(comm, group, local_rank, &local);           //Creation of local
    MPI_Comm_set_errhandler(local, MPI_ERRORS_RETURN);

    PMPI_Comm_split(comm, local_rank == 0, rank, &global);      //Creation of global
    if(local_rank != 0)
        PMPI_Comm_free(&global);
    else
        MPI_Comm_set_errhandler(global, MPI_ERRORS_RETURN);


    if(rank == 0)                                               //Creation of partially overlapped
    {
        if(size <= DIMENSION)
            PMPI_Comm_dup(local, &partially_overlapped_own);
        else
        {
            MPI_Comm icomm;
            int remote_leader = (size-1) / DIMENSION;
            MPI_Intercomm_create(MPI_COMM_SELF, 0, global, remote_leader, 40 + remote_leader, &icomm);
            MPI_Intercomm_merge(icomm, 1, &partially_overlapped_other);
            MPI_Comm_set_errhandler(partially_overlapped_other, MPI_ERRORS_RETURN);
            PMPI_Comm_free(&icomm);
            MPI_Intercomm_create(local, 0, global, 1, 40 + group, &icomm);
            MPI_Intercomm_merge(icomm, 0, &partially_overlapped_own);
            MPI_Comm_set_errhandler(partially_overlapped_own, MPI_ERRORS_RETURN);
            PMPI_Comm_free(&icomm);
        }
    }
    else
    {
        MPI_Comm icomm;
        int remote_leader = ((group+1) * DIMENSION < size ? group + 1 : 0);
        if(size <= DIMENSION)
        {
            PMPI_Comm_dup(local, &partially_overlapped_own);
        }
        else
        {
            MPI_Intercomm_create(local, 0, global, remote_leader, 40 + group, &icomm);
            MPI_Intercomm_merge(icomm, 0, &partially_overlapped_own);
            MPI_Comm_set_errhandler(partially_overlapped_own, MPI_ERRORS_RETURN);
            PMPI_Comm_free(&icomm);

            if(local_rank == 0)
            {
                remote_leader = group-1; //cannot underflow since I'm handling 0 separately
                MPI_Intercomm_create(MPI_COMM_SELF, 0, global, remote_leader, 40 + remote_leader, &icomm);
                MPI_Intercomm_merge(icomm, 1, &partially_overlapped_other);
                MPI_Comm_set_errhandler(partially_overlapped_other, MPI_ERRORS_RETURN);
                PMPI_Comm_free(&icomm);
            }
            else
                partially_overlapped_other = MPI_COMM_NULL;
        }
    }

    //Creation of notifier thread
    shrink_check = new std::thread(&HierarComm::change_even_if_unnotified, this, group);    

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
    print_status();
}

void HierarComm::fault_manage(MPI_Comm problematic) 
{
    int rank;
    MPI_Comm_rank(get_alias(), &rank);
    if(MPI_Comm_c2f(problematic) == MPI_Comm_c2f(local))
    {
        print_details("There is a local fault ._.", rank);
        local_fault_manage();
    }
    else if(MPI_Comm_c2f(problematic) == MPI_Comm_c2f(full_network))
    {
        print_details("There is a full fault u_u", rank);
        full_network_fault_manage();
    }
    else if(MPI_Comm_c2f(problematic) == MPI_Comm_c2f(global))
    {
        print_details("There is a global fault o_o", rank);
        global_fault_manage();
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
    int new_rank = translate_ranks(rank, full_network);
    return op(new_rank, full_network, this);
}

int HierarComm::perform_operation(OneToAll op, int rank) 
{
    if(!op.isPositional())
    {
        int root_group = extract_group(rank), this_group, this_rank, this_local_rank;
        int rc;

        MPI_Comm_rank(get_alias(), &this_rank);
        this_group = extract_group(this_rank);
        if(root_group == this_group)
        {
            //root is local
            do
            {
                PMPI_Barrier(local);            //This Barrier is needed since otherwise consistent error detection may not be reached
                int local_rank = translate_ranks(rank, local);
                rc = op(local_rank, local, this);
            } while(rc != MPI_SUCCESS);

            MPI_Comm_rank(local, &this_local_rank);
            if(this_local_rank == 0)
            {
                //I'm master in this subnet, need to operate on global
                do
                {
                    PMPI_Barrier(global);
                    int local_rank = translate_ranks(this_rank, global);
                    rc = op(local_rank, global, this);
                } while(rc != MPI_SUCCESS);
            }
        }
        else
        {
            MPI_Comm_rank(local, &this_local_rank);
            if(this_local_rank == 0)
            {
                //I'm master in this subnet, need to propagate
                do
                {
                    PMPI_Barrier(global);
                    int source_rank = MPI_UNDEFINED;
                    for(int i = root_group * DIMENSION; i < (root_group + 1) * DIMENSION && source_rank == MPI_UNDEFINED; i++)
                    {
                        source_rank = translate_ranks(i, global);
                    }
                    rc = op(source_rank, global, this);
                } while(rc != MPI_SUCCESS);
            }

            //root is in another subnet, need to perform using master as root
            do
            {
                rc = op(0, local, this);
            } while(rc != MPI_SUCCESS);
        }
        return rc;
    }
    else
    {
        int rc;
        do
        {
            rc = op(translate_ranks(rank, full_network), full_network, this);
        } while(rc != MPI_SUCCESS);
        return rc;
    }
    
}

int HierarComm::perform_operation(AllToOne op, int rank) 
{
    if(!op.isPositional())
    {
        int root_group = extract_group(rank), this_group, this_rank, this_local_rank;
        int rc;

        MPI_Comm_rank(get_alias(), &this_rank);
        this_group = extract_group(this_rank);

        if(root_group != this_group)
        {
            //root is not local, aggregating to master
            do
            {            
                rc = op(0, local, this);
            } while(rc != MPI_SUCCESS);

            MPI_Comm_rank(local, &this_local_rank);
            if(this_local_rank == 0)
            {
                //I'm master in this subnet, must propagate the results
                int dest_rank = MPI_UNDEFINED;
                for(int i = root_group * DIMENSION; i < (root_group + 1) * DIMENSION && dest_rank == MPI_UNDEFINED; i++)
                    dest_rank = translate_ranks(i, global);
                do
                {
                    rc = op(dest_rank, global, this);
                } while(rc != MPI_SUCCESS);
            }
        }
        else
        {
            MPI_Comm_rank(local, &this_local_rank);
            if(this_local_rank == 0)
            {
                //I'm master, need to receive results
                do
                {
                    rc = op(translate_ranks(this_rank, global), global, this);
                } while(rc != MPI_SUCCESS);
            }

            do
            {
                int local_rank = translate_ranks(rank, local);
                rc = op(local_rank, local, this);
            } while(rc != MPI_SUCCESS);
        }
        return rc;
    }
    else
    {
        int rc;
        do
        {
            rc = op(translate_ranks(rank, full_network), full_network, this);
        } while(rc != MPI_SUCCESS);
        return rc;
    }
}

int HierarComm::perform_operation(AllToAll op) 
{
    if(!op.isPositional())
    {
        OneToAll half_barrier([] (int root, MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_Barrier(comm_t);
            if(rc != MPI_SUCCESS)
                replace_comm(adv, comm_t);
            return rc;
        }, false);

        perform_operation(half_barrier, 0);

        int local_rank;                     //Rank in local
        MPI_Group global_group;             //MPI_Group of global
        int pivot_rank;                     //Lowest rank alive

        MPI_Comm_rank(local, &local_rank);
        if(local_rank == 0)
        {
            int source = 0;                 //Lowest rank alive
            MPI_Comm_group(global, &global_group);
            MPI_Group_translate_ranks(global_group, 1, &source, get_group(), &pivot_rank);
            PMPI_Bcast(&pivot_rank, 1, MPI_INT, 0, local);
        }
        else
        {
            PMPI_Bcast(&pivot_rank, 1, MPI_INT, 0, local);   
        }
        
        int rc = perform_operation(op.decomp().first, pivot_rank);
        rc |= perform_operation(op.decomp().second, pivot_rank);
        return rc;
    }
    else
    {
        int rc;
        do
        {
            rc = op(full_network, this);
        } while(rc != MPI_SUCCESS);
        return rc;
    }
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
    int rc;
    do
    {
        rc = op(full_network, this);
    } while(rc != MPI_SUCCESS);
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

void HierarComm::local_fault_manage()
{
    MPI_Comm new_comm;                                  //It will substitute local
    int old_size;                                       //Size of old local
    int new_size;                                       //Size of shrinked local
    int old_rank;                                       //Rank in old local
    int new_rank;                                       //Rank in shrinked local
    int rank;                                           //Rank in alias

    MPIX_Comm_shrink(local, &new_comm);
    MPI_Comm_size(local, &old_size);
    MPI_Comm_size(new_comm, &new_size);
    MPI_Comm_rank(local, &old_rank);
    MPI_Comm_rank(new_comm, &new_rank);
    MPI_Comm_rank(get_alias(), &rank);

    int diff = old_size - new_size; // number of deads //
    if(0 == diff)
        PMPI_Comm_free(&new_comm);                      //Nobody failed, stop;
    else
    {
        MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);   //new_comm will become local

        MPI_Group old_group;                            //MPI_Group of failed local
        MPI_Group new_group;                            //MPI_Group of new local
        MPI_Group diff_group;                           //Difference of the 2 above

        MPI_Comm_group(local, &old_group);
        MPI_Comm_group(new_comm, &new_group);
        MPI_Group_difference(old_group, new_group, &diff_group);

        int failed_rank, source = 0;
        MPI_Group_translate_ranks(diff_group, 1, &source, old_group, &failed_rank);
        if(failed_rank == 0)
        {
            print_details("Shrinking own", rank);
            MPI_Comm temp;
            MPIX_Comm_shrink(partially_overlapped_own, &temp);
            PMPI_Comm_free(&partially_overlapped_own);
            partially_overlapped_own = temp;
        }

        if(old_rank != 0 && new_rank == 0)
        {
            // Master failed, I must join the global group
            print_details("Master failed, I must join the global group", rank);
            MPI_Comm iglobal;                               //global intercommunicator
            int rank_group;                                 //group of the process

            rank_group = extract_group(rank);

            print_details("I must use par_over_own to join global", rank);
            //The process must use partially_overlapped to join global
            int partially_overlapped_size;
            int partially_external_root, partially_overlapped_root, partially_external_group;
            MPI_Group partially_overlapped_group;
            MPI_Comm_size(partially_overlapped_own, &partially_overlapped_size);
            partially_overlapped_root = partially_overlapped_size - 1;
            MPI_Comm_group(partially_overlapped_own, &partially_overlapped_group);
            MPI_Group_translate_ranks(partially_overlapped_group, 1, &partially_overlapped_root, get_group(), &partially_external_root);
            partially_external_group = extract_group(partially_external_root);

            if(partially_external_group != rank_group)
            {
                print_details("Creating global icomm tag: " + std::to_string(40+partially_external_group), rank);

                MPI_Intercomm_create(MPI_COMM_SELF, 0, partially_overlapped_own, partially_overlapped_root, 40 + partially_external_group, &iglobal);
                PMPI_Comm_free(&global);

                MPI_Comm temp_intracomm;
                PMPI_Intercomm_merge(iglobal, 0, &temp_intracomm);
                print_details("Need to reshuffle indexes in global icomm", rank);
                //Here a intracommunicator exists, but the ranks are shuffled

                PMPI_Comm_split(temp_intracomm, 0, rank, &global);
                MPI_Comm_set_errhandler(global, MPI_ERRORS_RETURN);

                PMPI_Comm_free(&temp_intracomm);
                PMPI_Comm_free(&iglobal);

                //Now i must join another partial overlapping intercomm
                //To do so the newly created global is used
                print_details("I must join another par_over", rank);
                MPI_Group global_group;         //MPI_Group of global
                int global_rank;                //Rank in global
                int global_size;                //Size of global
                int prev_rank;                  //Rank of previous in global
                int abs_prev_rank;              //Rank of previous in alias
                int prev_group;                 //Group of previous
                
                MPI_Comm_group(global, &global_group);
                MPI_Comm_rank(global, &global_rank);
                MPI_Comm_size(global, &global_size);
                prev_rank = (global_rank - 1 + global_size) % global_size;
                MPI_Group_translate_ranks(global_group, 1, &prev_rank, get_group(), &abs_prev_rank);
                prev_group = extract_group(abs_prev_rank);

                print_details("Creating par_over intercomm token: " + std::to_string(50 + prev_group), rank);
                MPI_Intercomm_create(MPI_COMM_SELF, 0, global, prev_rank, 50 + prev_group, &temp_intracomm);
                MPI_Intercomm_merge(temp_intracomm, 1, &partially_overlapped_other);
                MPI_Comm_set_errhandler(partially_overlapped_other, MPI_ERRORS_RETURN);
                PMPI_Comm_free(&temp_intracomm);
            }
            else
            {
                PMPI_Comm_dup(MPI_COMM_SELF, &global);
                //Partially overlapped other is no more needed
            }
        }
        local_replace_comm(new_comm);
    }
    print_details("Local failure solved", rank);
    print_status();
}

void HierarComm::global_fault_manage()
{
    MPI_Comm new_comm;                                      //Shrinked global communicator
    int old_size;                                           //Size of old global
    int new_size;                                           //Size in shrinked global
    int old_rank;                                           //Rank in old global
    int new_rank;                                           //Rank in shrinked global
    int global_rank;                                        //Rank in alias
    int rank_group;                                         //group of the process

    MPIX_Comm_shrink(global, &new_comm);
    MPI_Comm_size(global, &old_size);
    MPI_Comm_size(new_comm, &new_size);
    MPI_Comm_rank(global, &old_rank);
    MPI_Comm_rank(new_comm, &new_rank);
    MPI_Comm_rank(get_alias(), &global_rank);
    rank_group = extract_group(global_rank);

    print_details("Starting procedure for global fix", global_rank);
    
    int diff = old_size - new_size; // number of deads
    if(0 == diff)
    {
        print_details("No processes exiting, strange...", global_rank);
        PMPI_Comm_free(&new_comm);
    }
    else
    {
        MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
        //There are 3 cases in which we can be:
        //The dead process is the one of the previous group;
        //The dead process is the one of the following group;
        //The dead process is neither the previous nor the following group.
        //The first thing to do is identify the case.

        MPI_Group old_group;                                //MPI_Group of old global
        MPI_Group new_group;                                //MPI_Group of new global
        MPI_Group diff_group;                               //Difference of the 2 above

        MPI_Comm_group(global, &old_group);
        MPI_Comm_group(new_comm, &new_group);
        MPI_Group_difference(old_group, new_group, &diff_group);
        
        int failed, source = 0;                             //failed contains the rank of the failed process in old global
        int failed_rank;                                    //rank of the failed process in alias
        int failed_group;                                   //group of the failed process
        int next_in_global;                                 //rank of the next process in old global
        int prev_in_global;                                 //rank of the previous process in old global
        int message;                                        //used for broadcast communication

        MPI_Group_translate_ranks(diff_group, 1, &source, old_group, &failed);
        MPI_Group_translate_ranks(diff_group, 1, &source, get_group(), &failed_rank);
        failed_group = extract_group(failed_rank);
        next_in_global = (failed + 1) % old_size;           //The rank next in global to the one failed (case 1)
        prev_in_global = (failed -1 + old_size) % old_size; //The rank prev in global to the one failed (case 2)

        //Note that next_in_global == prev_in_global iff old_size = 2 or old_size = 1
        //But if old_size = 1 then global failure is not handled here (there is no other global) :D

        if(old_rank == next_in_global)
        {
            print_details("Global fix case 1 begin", global_rank);
            //case 1
            MPI_Comm temp;
            print_details("About to shrink", global_rank);
            MPIX_Comm_shrink(partially_overlapped_other, &temp);
            PMPI_Comm_free(&partially_overlapped_other);
            
            int size_of_partial;
            MPI_Comm_size(temp, &size_of_partial);
            if(size_of_partial == 1)
            {
                print_details("Impossible to elect new master, shrinking...", global_rank);
                //The whole group is failed, no need to work with intercomm in global
                message = 0;
                PMPI_Bcast(&message, 1, MPI_INT, new_rank, new_comm);
                PMPI_Comm_free(&global);
                global = new_comm;

                //Need to adjust the other inter
                if(new_size == 1)
                {
                    //Partially overlapped other is not used anymore.
                    PMPI_Comm_free(&temp);
                }
                else
                {
                    int new_other = (new_rank - 1 + new_size) % new_size;
                    int new_other_rank;
                    int new_other_group;
                    MPI_Group_translate_ranks(new_group, 1, &new_other, get_group(), &new_other_rank);
                    new_other_group = extract_group(new_other_rank);

                    MPI_Comm icomm;
                    print_details("Adjusting a par_over, token: " + std::to_string(50 + new_other_group), global_rank);
                    MPI_Intercomm_create(MPI_COMM_SELF, 0, global, new_other, 50 + new_other_group, &icomm);
                    MPI_Intercomm_merge(icomm, 1, &partially_overlapped_other);
                    MPI_Comm_set_errhandler(partially_overlapped_other, MPI_ERRORS_RETURN);
                }
            }
            else
            {
                print_details("New master can be elected", global_rank);
                message = 1;
                MPI_Comm icomm, unordered_global;
                PMPI_Comm_free(&global);
                PMPI_Bcast(&message, 1, MPI_INT, new_rank, new_comm);
                MPI_Intercomm_create(new_comm, new_rank, temp, 0, 40 + rank_group, &icomm);
                print_details("Creating new global icomm, tag: " + std::to_string(40 + rank_group), global_rank);
                MPI_Intercomm_merge(icomm, 0, &unordered_global);
                print_details("Reordering new global", global_rank);
                PMPI_Comm_split(unordered_global, 0, global_rank, &global);
                PMPI_Comm_free(&icomm);
                PMPI_Comm_free(&unordered_global);
                partially_overlapped_other = temp;
            }
            print_details("Global fix case 1 end", global_rank);

        }
        if(old_rank == prev_in_global)
        {
            //case 2
            //First thing to do is propagate the failure across the local group
            print_details("Global fix case 2 begin", global_rank);
            int local_size, buf = 1;
            MPI_Request requests[DIMENSION];
            MPI_Status status[DIMENSION];
            MPI_Comm_size(local, &local_size);
            for(int i = 0; i + 1 < local_size; i++)
                PMPI_Issend(&buf, 1, MPI_INT, i + 1, 77, local, &(requests[i]));
            
            print_details("Local group notified via send", global_rank);
            //Then behave like case 3
            int rank_bcast_source, abs_bcast_source, group_bcast_source;
            MPI_Group_translate_ranks(old_group, 1, &next_in_global, new_group, &rank_bcast_source);
            MPI_Group_translate_ranks(old_group, 1, &next_in_global, get_group(), &abs_bcast_source);
            group_bcast_source = extract_group(abs_bcast_source);
            if(old_size > 2) //If i don't check this I would shrink twice in this case
            {
                PMPI_Bcast(&message, 1, MPI_INT, rank_bcast_source, new_comm);
                if(message == 0)
                {
                    print_details("No new master, shrinking...", global_rank);
                    PMPI_Comm_free(&global);
                    global = new_comm;
                }
                else
                {
                    print_details("New master exists", global_rank);
                    MPI_Comm icomm, unordered_global;
                    PMPI_Comm_free(&global);
                    MPI_Intercomm_create(new_comm, rank_bcast_source, MPI_COMM_NULL, 0, 40 + group_bcast_source, &icomm);
                    print_details("Creating new global icomm, tag: " + std::to_string(40 + group_bcast_source), global_rank);
                    MPI_Intercomm_merge(icomm, 0, &unordered_global);
                    print_details("Reordering new global", global_rank);
                    PMPI_Comm_split(unordered_global, 0, global_rank, &global);
                    PMPI_Comm_free(&icomm);
                    PMPI_Comm_free(&unordered_global);
                }
            }

            //And now reconstruct partially_overlapped_own
            int updated_global_rank;                            //Rank in the fixed communicator
            int updated_global_size;                            //Size of the fixed communicator
            MPI_Comm_rank(global, &updated_global_rank);
            MPI_Comm_size(global, &updated_global_size);
            int new_follower = (updated_global_rank + 1) % updated_global_size;

            PMPI_Waitall(local_size - 1, requests, status);

            print_details("All locals have been notified, can proceed with shrink", global_rank);

            MPI_Comm icomm, temp;
            MPIX_Comm_shrink(partially_overlapped_own, &temp);
            PMPI_Comm_free(&partially_overlapped_own);
            print_details("Creating own icomm, token: " + std::to_string(50+rank_group), global_rank);
            MPI_Intercomm_create(temp, 0, global, new_follower, 50 + rank_group, &icomm);
            MPI_Intercomm_merge(icomm, 0, &partially_overlapped_own);
            MPI_Comm_set_errhandler(partially_overlapped_own, MPI_ERRORS_RETURN);
            PMPI_Comm_free(&icomm);
            PMPI_Comm_free(&temp);
            print_details("Global fix case 2 end", global_rank);
        }
        if(old_rank != prev_in_global && old_rank != next_in_global)
        {
            //case 3
            print_details("Global fix case 3 begin", global_rank);
            int rank_bcast_source, abs_bcast_source, group_bcast_source;
            MPI_Group_translate_ranks(old_group, 1, &next_in_global, new_group, &rank_bcast_source);
            MPI_Group_translate_ranks(old_group, 1, &next_in_global, get_group(), &abs_bcast_source);
            group_bcast_source = extract_group(abs_bcast_source);
            PMPI_Bcast(&message, 1, MPI_INT, rank_bcast_source, new_comm);
            if(message == 0)
            {
                print_details("No new master, shrinking...", global_rank);
                PMPI_Comm_free(&global);
                global = new_comm;
            }
            else
            {
                print_details("New master exists", global_rank);
                MPI_Comm icomm, unordered_global;
                PMPI_Comm_free(&global);
                MPI_Intercomm_create(new_comm, rank_bcast_source, MPI_COMM_NULL, 0, 40 + group_bcast_source, &icomm);
                print_details("Creating new global icomm, tag: " + std::to_string(40 + group_bcast_source), global_rank);
                MPI_Intercomm_merge(icomm, 0, &unordered_global);
                print_details("Reordering new global", global_rank);
                PMPI_Comm_split(unordered_global, 0, global_rank, &global);
                PMPI_Comm_free(&icomm);
                PMPI_Comm_free(&unordered_global);
            }
            print_details("Global fix case 3 end", global_rank);
        }
    }
    print_status();
}

void HierarComm::full_network_fault_manage()
{
    MPI_Comm new_comm;
    int old_size, new_size, diff;
    MPIX_Comm_shrink(full_network, &new_comm);
    MPI_Comm_size(full_network, &old_size);
    MPI_Comm_size(new_comm, &new_size);
    diff = old_size - new_size; // number of deads
    if(0 == diff)
        PMPI_Comm_free(&new_comm);
    else
    {
        PMPI_Comm_free(&full_network);
        MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
        full_network = new_comm;
    }
}

void print_details(std::string details, int rank)
{
    if(PRINT_DETAILS)
    {
        printf("[%d] %s\n", rank, details.c_str());
    }
}

void HierarComm::print_status()
{
    if(PRINT_DETAILS)
    {
        int global_rank;
        MPI_Comm_rank(get_alias(), &global_rank);
        std::string base = "";

        int local_size;
        MPI_Group local_group;
        MPI_Comm_group(local, &local_group);
        MPI_Group_size(local_group, &local_size);
        for(int i = 0; i < local_size; i++)
        {
            int target;
            MPI_Group_translate_ranks(local_group, 1, &i, get_group(), &target);
            base += std::to_string(target) + " ";
        }
        print_details("Local processes: " + base, global_rank);
        base = "";

        MPI_Comm_group(partially_overlapped_own, &local_group);
        MPI_Group_size(local_group, &local_size);
        for(int i = 0; i < local_size; i++)
        {
            int target;
            MPI_Group_translate_ranks(local_group, 1, &i, get_group(), &target);
            base += std::to_string(target) + " ";
        }
        print_details("Own processes: " + base, global_rank);
        base = "";

        if(global != MPI_COMM_NULL)
        {
            MPI_Comm_group(global, &local_group);
            MPI_Group_size(local_group, &local_size);
            for(int i = 0; i < local_size; i++)
            {
                int target;
                MPI_Group_translate_ranks(local_group, 1, &i, get_group(), &target);
                base += std::to_string(target) + " ";
            }
            print_details("Global processes: " + base, global_rank);
            base = "";

            MPI_Comm_group(partially_overlapped_other, &local_group);
            MPI_Group_size(local_group, &local_size);
            for(int i = 0; i < local_size; i++)
            {
                int target;
                MPI_Group_translate_ranks(local_group, 1, &i, get_group(), &target);
                base += std::to_string(target) + " ";
            }
            print_details("Other processes: " + base, global_rank);
        }
    }
}