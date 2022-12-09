#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <functional>
#include <unordered_map>
#include "mpi.h"
#include "structure_handler.hpp"

namespace legio {

class RequestHandler : public StructureHandler<MPI_Request, MPI_Comm>
{
   public:
    RequestHandler(std::function<int(MPI_Request, int*)>,
                   std::function<int(MPI_Request, int*, int*)>,
                   std::function<int(MPI_Request*)>,
                   std::function<int(MPI_Request, MPI_Request*)>,
                   int);
    void replace(MPI_Comm) override;
};

}  // namespace legio

#endif