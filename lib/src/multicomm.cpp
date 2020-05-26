#include <unordered_map>
#include <stdio.h>
#include "complex_comm.h"
#include "multicomm.h"
#include "mpi.h"

Multicomm::Multicomm()
{}

void Multicomm::add_comm(MPI_Comm added)
{
    int id = MPI_Comm_c2f(added);
    MPI_Comm temp;
    PMPI_Comm_dup(added, &temp);
    MPI_Comm_set_errhandler(temp, MPI_ERRORS_RETURN);
    std::pair<int, ComplexComm> adding(id, ComplexComm(temp, id));
    comms.insert(adding);
    std::unordered_map<int, ComplexComm>::iterator res = comms.find(id);
}

ComplexComm* Multicomm::translate_into_complex(MPI_Comm input)
{
    std::unordered_map<int, ComplexComm>::iterator res = comms.find(MPI_Comm_c2f(input));
    if(res == comms.end())
    {
        printf("THIS SHOULDN'T HAVE HAPPENED, USE part_of BEFORE TRANSLATE.\n");
        return NULL;
    }
    else return &(res->second);
}

void Multicomm::remove(MPI_Comm removed, std::function<int(MPI_Comm*)> destroyer)
{
    int id = MPI_Comm_c2f(removed);
    std::unordered_map<int, ComplexComm>::iterator res = comms.find(id);
    if(res != comms.end())
    {
        MPI_Comm target = res->second.get_comm();

        destroyer(&target);

        comms.erase(id); 
    }
    else
    {
        printf("THIS SHOULDN'T HAVE HAPPENED, REMOVING A NEVER INSERTED COMM.\n");
    }    
}

void Multicomm::part_of(MPI_Comm checked, int* result)
{
    std::unordered_map<int, ComplexComm>::iterator res = comms.find(MPI_Comm_c2f(checked));
    *result = (res != comms.end());
}

void Multicomm::add_file(ComplexComm* comm, MPI_File file, std::function<int(MPI_Comm, MPI_File*)> func)
{
    comm->add_structure(file, func);
    int id = MPI_Comm_c2f(comm->get_alias());
    file_map.insert({MPI_File_c2f(file), id});
}

void Multicomm::add_window(ComplexComm* comm, MPI_Win win, std::function<int(MPI_Comm, MPI_Win *)> func)
{
    comm->add_structure(win, func);
    int id = MPI_Comm_c2f(comm->get_alias());
    window_map.insert({MPI_Win_c2f(win), id});
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
        auto res2 = comms.find(res->second);
        if(res2 != comms.end())
            return &(res2->second);
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
        auto res2 = comms.find(res->second);
        if(res2 != comms.end())
            return &(res2->second);
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