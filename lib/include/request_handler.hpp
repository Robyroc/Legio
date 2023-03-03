#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <functional>
#include <unordered_map>
#include "mpi.h"
#include "mpi_structs.hpp"
#include "structure_handler.hpp"

namespace legio {

class RequestHandler : public StructureHandler<Legio_request, MPI_Comm>
{
   public:
    RequestHandler(std::function<int(Legio_request, int*)>,
                   std::function<int(Legio_request, int*, int*)>,
                   std::function<int(Legio_request*)>,
                   std::function<int(Legio_request, Legio_request*)>,
                   int);
    void replace(MPI_Comm) override;
};

}  // namespace legio

#endif