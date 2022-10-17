#include <unordered_map>
#include <stdio.h>
#include "complex_comm.h"
#include "multicomm.h"
#include "mpi.h"
#include <functional>

Multicomm::Multicomm(int size){ 
    // Initialize ranks
    for (int i = 0; i < size; i++) {
        ranks.push_back(Rank(i, false));
    }
};

int Multicomm::add_comm(MPI_Comm added, MPI_Comm parent, std::function<int(MPI_Comm, MPI_Comm*)> generator, std::function<int(MPI_Comm, MPI_Comm, MPI_Comm*)> inter_generator, MPI_Comm second_parent)
{
    if (!respawned) {
        int id = MPI_Comm_c2f(added);
        MPI_Comm temp;
        PMPI_Comm_dup(added, &temp);
        MPI_Comm_set_errhandler(temp, MPI_ERRORS_RETURN);
        //std::pair<int, int> order_adding(id, size++);
        std::pair<int, ComplexComm> adding(id, ComplexComm(temp, id, MPI_Comm_c2f(parent), generator, inter_generator, MPI_Comm_c2f(second_parent)));
        auto res = comms.insert(adding);
        //comms_order.insert(order_adding);
        return res.second;
    }
    else {
        int id = MPI_Comm_c2f(added);
        //std::pair<int, int> order_adding(id, size++);
        std::pair<int, ComplexComm> adding(id, ComplexComm(added, id, MPI_Comm_c2f(parent), generator, inter_generator, MPI_Comm_c2f(second_parent)));
        auto res = comms.insert(adding);
        //comms_order.insert(order_adding);
        return res.second;
    }
}

ComplexComm* Multicomm::translate_into_complex(MPI_Comm input)
{
    /*
    std::unordered_map<int, int>::iterator res = comms_order.find(MPI_Comm_c2f(input));
    if(res == comms_order.end())
    {
        if(input != MPI_COMM_NULL)
            printf("THIS SHOULDN'T HAVE HAPPENED, USE part_of BEFORE TRANSLATE.\n");
        return NULL;
    }
    else
    {
        std::map<int, ComplexComm>::iterator res2 = comms.find(res->second);
        return &(res2->second);
    }
    */
    auto res = comms.find(MPI_Comm_c2f(input));
    if (res == comms.end())
    {
        if(input != MPI_COMM_NULL)
            printf("THIS SHOULDN'T HAVE HAPPENED, USE part_of BEFORE TRANSLATE.\n");
        return NULL;
    }
    else return &(res->second);
}

void Multicomm::remove(MPI_Comm removed, std::function<int(MPI_Comm*)> destroyer)
{
    int id = MPI_Comm_c2f(removed);
    auto res = comms.find(id);
    //std::unordered_map<int, int>::iterator res = comms_order.find(id);
    if(res != comms.end())
    {
        //std::map<int, ComplexComm>::iterator res2 = comms.find(res->second);

        res->second.destroy(destroyer);

        //Removed deletion since it may be useful for the recreation of the following comms
        comms.erase(id); 
    }
    else
    {
        printf("THIS SHOULDN'T HAVE HAPPENED, REMOVING A NEVER INSERTED COMM.\n");
    }    
}

void Multicomm::part_of(MPI_Comm checked, int* result)
{
    auto res = comms.find(MPI_Comm_c2f(checked));
    *result = (res != comms.end());
}

void Multicomm::remove_window(MPI_Win* win)
{
    ComplexComm* translated = get_complex_from_structure(*win);
    if(translated != NULL)
    {
        translated->remove_structure(*win);
        maps[handle_selector<MPI_Win>::get()].erase(MPI_Win_c2f(*win));
    }
    else
        PMPI_Win_free(win);
}

void Multicomm::remove_file(MPI_File* file)
{
    ComplexComm* translated = get_complex_from_structure(*file);
    if(translated != NULL)
    {
        translated->remove_structure(*file);
        maps[handle_selector<MPI_File>::get()].erase(MPI_File_c2f(*file));
    }
    else
        PMPI_File_close(file);
}

void Multicomm::remove_request(MPI_Request* req)
{
    ComplexComm* translated = get_complex_from_structure(*req);
    if(translated != NULL)
    {
        translated->remove_structure(*req);
        maps[handle_selector<MPI_Request>::get()].erase(c2f<MPI_Request>(*req));
    }
}

void Multicomm::change_comm(ComplexComm* current, MPI_Comm newcomm)
{
    current->replace_comm(newcomm);
}


// Translate ranks from source (current ranks) to destination (alias-related ranks)
void Multicomm::translate_ranks(int source_rank, ComplexComm* comm, int* dest_rank)
{
    MPI_Group tr_group;
    int source = source_rank;
    int failed_ranks = 0;
    auto res = supported_comms.find(comm->get_alias_id());
    if( res == supported_comms.end() && comm->get_alias() != MPI_COMM_WORLD) {
        MPI_Group tr_group;
        int source = source_rank;
        MPI_Comm_group(comm->get_comm(), &tr_group);
        MPI_Group_translate_ranks(comm->get_group(), 1, &source, tr_group, dest_rank);
        return;
    }
    else if (comm->get_alias() == MPI_COMM_WORLD) {
        for (int i = 0; i < source_rank; i++) {
            if (ranks.at(i).failed)
                failed_ranks++;
        }
        *dest_rank = source_rank - failed_ranks;
    }
    else {
        SupportedComm respawned_comm = res->second;
        failed_ranks = respawned_comm.get_failed_ranks_before(source_rank);
        *dest_rank = source_rank - failed_ranks;
    }
}
