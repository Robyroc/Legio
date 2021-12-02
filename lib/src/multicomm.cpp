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

bool Multicomm::add_file(ComplexComm* comm, MPI_File file, std::function<int(MPI_Comm, MPI_File*)> func)
{
    int id = MPI_Comm_c2f(comm->get_alias());
    auto res = file_map.insert({MPI_File_c2f(file), id});
    if(res.second) 
        comm->add_structure(file, func);
    return res.second;
}

bool Multicomm::add_window(ComplexComm* comm, MPI_Win win, std::function<int(MPI_Comm, MPI_Win *)> func)
{
    int id = MPI_Comm_c2f(comm->get_alias());
    auto res = window_map.insert({MPI_Win_c2f(win), id});
    if(res.second)
        comm->add_structure(win, func);
    return res.second;
}

void Multicomm::remove_window(MPI_Win* win)
{
    ComplexComm* translated = get_complex_from_win(*win);
    if(translated != NULL)
    {
        translated->remove_structure(*win);
        window_map.erase(MPI_Win_c2f(*win));
    }
    else
        PMPI_Win_free(win);
}

void Multicomm::remove_file(MPI_File* file)
{
    ComplexComm* translated = get_complex_from_file(*file);
    if(translated != NULL)
    {
        translated->remove_structure(*file);
        file_map.erase(MPI_File_c2f(*file));
    }
    else
        PMPI_File_close(file);
}

ComplexComm* Multicomm::get_complex_from_win(MPI_Win win)
{
    std::unordered_map<int, int>::iterator res = window_map.find(MPI_Win_c2f(win));
    if(res != window_map.end())
    {
        std::unordered_map<int, int>::iterator res2 = comms_order.find(res->second);
        if(res2 != comms_order.end())
            return &(comms.find(res2->second)->second);
        else
        {
            printf("IMPOSSIBLE BEHAVIOUR!!!\n");
            return NULL;
        }
    }
    else
    {
        printf("WIN NOT MAPPED\n");
        return NULL;
    }
}

ComplexComm* Multicomm::get_complex_from_file(MPI_File file)
{
    std::unordered_map<int, int>::iterator res = file_map.find(MPI_File_c2f(file));
    if(res != file_map.end())
    {
        std::unordered_map<int, int>::iterator res2 = comms_order.find(res->second);
        if(res2 != comms_order.end())
            return &(comms.find(res2->second)->second);
        else
        {
            printf("IMPOSSIBLE BEHAVIOUR!!!\n");
            return NULL;
        }
    }
    else
    {
        printf("FILE NOT MAPPED\n");
        return NULL;
    }
}

void Multicomm::change_comm(ComplexComm* current, MPI_Comm newcomm)
{
    current->replace_comm(newcomm);
}