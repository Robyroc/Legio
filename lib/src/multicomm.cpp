#include <unordered_map>
#include <stdio.h>
#include "complex_comm.h"
#include "multicomm.h"
#include "mpi.h"
#include <functional>

Multicomm::Multicomm() : size(0)
{}

int Multicomm::add_comm(MPI_Comm added, MPI_Comm parent, std::function<int(MPI_Comm, MPI_Comm*)> generator, std::function<int(MPI_Comm, MPI_Comm, MPI_Comm*)> inter_generator, MPI_Comm second_parent)
{
    int id = MPI_Comm_c2f(added);
    MPI_Comm temp;
    PMPI_Comm_dup(added, &temp);
    MPI_Comm_set_errhandler(temp, MPI_ERRORS_RETURN);
    std::pair<int, int> order_adding(id, size++);
    std::pair<int, ComplexComm> adding(order_adding.second, ComplexComm(temp, id, MPI_Comm_c2f(parent), generator, inter_generator, MPI_Comm_c2f(second_parent)));
    auto res = comms.insert(adding);
    comms_order.insert(order_adding);
    return res.second;
}

ComplexComm* Multicomm::translate_into_complex(MPI_Comm input)
{
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
}

void Multicomm::remove(MPI_Comm removed, std::function<int(MPI_Comm*)> destroyer)
{
    int id = MPI_Comm_c2f(removed);
    std::unordered_map<int, int>::iterator res = comms_order.find(id);
    if(res != comms_order.end())
    {
        std::map<int, ComplexComm>::iterator res2 = comms.find(res->second);

        res2->second.destroy(destroyer);

        //Removed deletion since it may be useful for the recreation of the following comms
        //comms.erase(id); 
    }
    else
    {
        printf("THIS SHOULDN'T HAVE HAPPENED, REMOVING A NEVER INSERTED COMM.\n");
    }    
}

void Multicomm::part_of(MPI_Comm checked, int* result)
{
    std::unordered_map<int, int>::iterator res = comms_order.find(MPI_Comm_c2f(checked));
    *result = (res != comms_order.end());
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