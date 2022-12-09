#include "request_handler.hpp"
#include "functional"
#include "mpi.h"
#include "unordered_map"

using namespace legio;

void RequestHandler::replace(MPI_Comm new_comm)
{
    MPI_Request temp;
    for (auto it = opened.begin(); it != opened.end(); it++)
    {
        temp = it->second.second;
        int flag;
        PMPI_Test(&temp, &flag, MPI_STATUS_IGNORE);
        if (!flag)
        {
            destroyer(&(it->second.second));
            it->second.first(new_comm, &(it->second.second));
            attribute_set(it->second.second, (int*)&(it->first));
            adapt(temp, &(it->second.second));
        }
    }
}

RequestHandler::RequestHandler(std::function<int(MPI_Request, int*)> setter,
                               std::function<int(MPI_Request, int*, int*)> getter,
                               std::function<int(MPI_Request*)> killer,
                               std::function<int(MPI_Request, MPI_Request*)> adapter,
                               int flag)
    : StructureHandler<MPI_Request, MPI_Comm>(setter, getter, killer, adapter, flag)
{
}