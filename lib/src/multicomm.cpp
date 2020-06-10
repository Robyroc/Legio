#include <unordered_map>
#include <stdio.h>
#include "adv_comm.h"
#include "multicomm.h"
#include "mpi.h"
#include "no_comm.h"

Multicomm::Multicomm()
{}

AdvComm* Multicomm::translate_into_adv(MPI_Comm input)
{
    std::unordered_map<int, AdvComm*>::iterator res = comms.find(MPI_Comm_c2f(input));
    if(res == comms.end())
    {
        printf("THIS SHOULDN'T HAVE HAPPENED, USE part_of BEFORE TRANSLATE.\n");
        return new NoComm(input);
    }
    else return res->second;
}

void Multicomm::remove(MPI_Comm removed, std::function<int(MPI_Comm*)> destroyer)
{
    int id = MPI_Comm_c2f(removed);
    std::unordered_map<int, AdvComm*>::iterator res = comms.find(id);
    if(res != comms.end())
    {
        res->second->destroy(destroyer);
        delete res->second;
        comms.erase(id); 
    }
    else
    {
        printf("THIS SHOULDN'T HAVE HAPPENED, REMOVING A NEVER INSERTED COMM.\n");
    }    
}

void Multicomm::part_of(MPI_Comm checked, int* result)
{
    std::unordered_map<int, AdvComm*>::iterator res = comms.find(MPI_Comm_c2f(checked));
    *result = (res != comms.end());
}

//  FILE AND WINDOWS MANIPULATION

bool Multicomm::add_file(AdvComm* comm, MPI_File file, std::function<int(MPI_Comm, MPI_File*)> func)
{
    if(comm->file_support())
    {
        int id = MPI_Comm_c2f(comm->get_alias());
        auto res = file_map.insert({MPI_File_c2f(file), id});
        if(res.second) 
            comm->add_structure(file, func);
        return res.second;
    }
    else
    {
        printf("Files not supported\n");
        return true;
    }
}

bool Multicomm::add_window(AdvComm* comm, MPI_Win win, std::function<int(MPI_Comm, MPI_Win *)> func)
{
    if(comm->window_support())
    {
        int id = MPI_Comm_c2f(comm->get_alias());
        auto res = window_map.insert({MPI_Win_c2f(win), id});
        if(res.second)
            comm->add_structure(win, func);
        return res.second;
    }
    else
    {
        printf("Windows not supported\n");
        return true;
    }
}

void Multicomm::remove_window(MPI_Win* win)
{
    AdvComm* translated = get_adv_from_win(*win);
    if(translated != NULL && translated->window_support())
    {
        translated->remove_structure(*win);
        window_map.erase(MPI_Win_c2f(*win));
    }
    else
        PMPI_Win_free(win);
}

void Multicomm::remove_file(MPI_File* file)
{
    AdvComm* translated = get_adv_from_file(*file);
    if(translated != NULL && translated->file_support())
    {
        translated->remove_structure(*file);
        file_map.erase(MPI_File_c2f(*file));
    }
    else
        PMPI_File_close(file);
}

AdvComm* Multicomm::get_adv_from_win(MPI_Win win)
{
    std::unordered_map<int, int>::iterator res = window_map.find(MPI_Win_c2f(win));
    if(res != window_map.end())
    {
        auto res2 = comms.find(res->second);
        if(res2 != comms.end())
            return res2->second;
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

AdvComm* Multicomm::get_adv_from_file(MPI_File file)
{
    std::unordered_map<int, int>::iterator res = file_map.find(MPI_File_c2f(file));
    if(res != file_map.end())
    {
        auto res2 = comms.find(res->second);
        if(res2 != comms.end())
            return res2->second;
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