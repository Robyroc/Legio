#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <functional>
#include <unordered_map>
#include "structure_handler.h"
#include "mpi.h"

class RequestHandler : public StructureHandler<MPI_Request, MPI_Comm>
{
    public:
        RequestHandler(std::function<int(MPI_Request,int*)>, std::function<int(MPI_Request,int*, int*)>, std::function<int(MPI_Request*)>, std::function<int(MPI_Request, MPI_Request*)>, int);
        virtual void replace(MPI_Comm);    
};


#endif