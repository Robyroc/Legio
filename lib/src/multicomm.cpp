#include "multicomm.hpp"
#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include "complex_comm.hpp"
#include "config.hpp"
#include "mpi.h"

using namespace legio;

int Multicomm::add_comm(MPI_Comm added)
{
    // assert(initialized);
    int id = c2f<MPI_Comm>(added);
    MPI_Comm temp;
    PMPI_Comm_dup(added, &temp);
    MPI_Comm_set_errhandler(temp, MPI_ERRORS_RETURN);
    std::pair<int, ComplexComm> adding(id, ComplexComm(temp, id));
    auto res = comms.insert(adding);
    return res.second;
}

ComplexComm& Multicomm::translate_into_complex(MPI_Comm input)
{
    // assert(initialized);
    auto res = comms.find(c2f<MPI_Comm>(input));
    if (res == comms.end())
    {
        if (input != MPI_COMM_NULL)
            assert(false && "THIS SHOULDN'T HAVE HAPPENED, USE part_of BEFORE TRANSLATE.\n");
        throw std::invalid_argument("USE part_of BEFORE TRANSLATE.\n");
    }
    else
        return res->second;
}

void Multicomm::remove(MPI_Comm removed, std::function<int(MPI_Comm*)> destroyer)
{
    // assert(initialized);
    int id = c2f<MPI_Comm>(removed);
    auto res = comms.find(id);
    // std::unordered_map<int, int>::iterator res = comms_order.find(id);
    if (res != comms.end())
    {
        MPI_Comm target = res->second.get_comm();
        destroyer(&target);
        comms.erase(id);
    }
    else
    {
        assert(false && "THIS SHOULDN'T HAVE HAPPENED, REMOVING A NEVER INSERTED COMM.\n");
    }
}

const bool Multicomm::part_of(MPI_Comm checked) const
{
    // assert(initialized);
    auto res = comms.find(c2f<MPI_Comm>(checked));
    return res != comms.end();
}

void Multicomm::remove_structure(MPI_Win* win)
{
    // assert(initialized);
    if (part_of(*win))
    {
        ComplexComm& translated = get_complex_from_structure(*win);
        translated.remove_structure(*win);
        maps[handle_selector<MPI_Win>::get()].erase(MPI_Win_c2f(*win));
    }
    else
        PMPI_Win_free(win);
}

void Multicomm::remove_structure(MPI_File* file)
{
    // assert(initialized);
    if (part_of(*file))
    {
        ComplexComm& translated = get_complex_from_structure(*file);
        translated.remove_structure(*file);
        maps[handle_selector<MPI_File>::get()].erase(MPI_File_c2f(*file));
    }
    else
        PMPI_File_close(file);
}

void Multicomm::remove_structure(MPI_Request* req)
{
    // assert(initialized);
    if (part_of(*req))
    {
        ComplexComm& translated = get_complex_from_structure(*req);
        translated.remove_structure(*req);
        maps[handle_selector<MPI_Request>::get()].erase(c2f<MPI_Request>(*req));
    }
}
