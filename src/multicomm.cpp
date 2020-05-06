#include <unordered_map>
#include <stdio.h>
#include "complex_comm.h"
#include "multicomm.h"
#include "mpi.h"

Multicomm::Multicomm()
{
    MPI_Comm_create_keyval(MPI_COMM_NULL_COPY_FN, MPI_COMM_NULL_DELETE_FN, &keyval, (void*)0);
}

void Multicomm::add_comm(MPI_Comm added)
{
    int id = counter++;
    MPI_Comm temp;
    PMPI_Comm_dup(added, &temp);
    MPI_Comm_set_errhandler(temp, MPI_ERRORS_RETURN);
    std::pair<int, ComplexComm> adding(id, ComplexComm(temp));
    comms.insert(adding);
    std::unordered_map<int, ComplexComm>::iterator res = comms.find(id);
    MPI_Comm_set_attr(added, keyval, (int*) &(res->first));
    MPI_Comm_set_attr(res->second.get_comm(), keyval, (int*) &(res->first));
}

ComplexComm* Multicomm::translate_into_complex(MPI_Comm input)
{
    int value;
    int* pointer = &value;
    int flag = 0;
    MPI_Comm_get_attr(input, keyval, &pointer, &flag);
    if(flag)
    {
        std::unordered_map<int, ComplexComm>::iterator res = comms.find(*pointer);
        if(res == comms.end())
        {
            return NULL;
        }
        else return &(res->second);
    }
    else
    {
        printf("THIS SHOULDN'T HAVE HAPPENED1...\n");
        return NULL;
    }
}

void Multicomm::remove(MPI_Comm removed)
{
    int key;
    int* pointer = &key;
    int flag;
    MPI_Comm_get_attr(removed, keyval, &pointer, &flag);
    if(flag)
    {
        std::unordered_map<int, ComplexComm>::iterator res = comms.find(*pointer);
        if(res != comms.end())
        {
            MPI_Comm target = res->second.get_comm();

            PMPI_Comm_free(&target);
        }
        comms.erase(*pointer);
    }
    else
    {
        printf("THIS SHOULDN'T HAVE HAPPENED2...\n");
    }
    
}

void Multicomm::part_of(MPI_Comm checked, int* result)
{
    int value;
    int* pointer = &value;
    MPI_Comm_get_attr(checked, keyval, &pointer, result);
}

void Multicomm::add_file(ComplexComm* comm, MPI_File file, std::function<int(MPI_Comm, MPI_File*)> func)
{
    comm->add_structure(file, func);
    int value, flag;
    int* pointer = &value;
    MPI_Comm_get_attr(comm->get_comm(), keyval, &pointer, &flag);
    window_map.insert({MPI_File_c2f(file), *pointer});
}

void Multicomm::add_window(ComplexComm* comm, MPI_Win win, std::function<int(MPI_Comm, MPI_Win *)> func)
{
    comm->add_structure(win, func);
    int value, flag;
    int* pointer = &value;
    MPI_Comm_get_attr(comm->get_comm(), keyval, &pointer, &flag);
    window_map.insert({MPI_Win_c2f(win), *pointer});
}

void Multicomm::remove_window(MPI_Win* win)
{
    ComplexComm* translated = get_complex_from_win(*win);
    if(translated != NULL)
        translated->remove_structure(*win);
    else
        PMPI_Win_free(win);
}

void Multicomm::remove_file(MPI_File* file)
{
    ComplexComm* translated = get_complex_from_file(*file);
    if(translated != NULL)
        translated->remove_structure(*file);
    else
        PMPI_File_close(file);
}

ComplexComm* Multicomm::get_complex_from_win(MPI_Win win)
{
    std::unordered_map<int, int>::iterator res = window_map.find(MPI_Win_c2f(win));
    if(res != window_map.end())
        return &(comms.find(res->second)->second);
    else
        return NULL;
}

ComplexComm* Multicomm::get_complex_from_file(MPI_File file)
{
    std::unordered_map<int, int>::iterator res = file_map.find(MPI_File_c2f(file));
    if(res != file_map.end())
        return &(comms.find(res->second)->second);
    else
        return NULL;
}

void Multicomm::change_comm(ComplexComm* current, MPI_Comm newcomm)
{
    int value, flag;
    int* pointer = &value;
    MPI_Comm_get_attr(current->get_comm(), keyval, &pointer, &flag);
    std::unordered_map<int, ComplexComm>::iterator res = comms.find(*pointer);
    MPI_Comm_set_attr(newcomm, keyval, (int*) &(res->first));
    current->replace_comm(newcomm);
}