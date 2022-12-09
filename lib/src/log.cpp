#include "log.hpp"
#include <iostream>
#include <string>
#include "config.hpp"
#include "mpi.h"

namespace legio {
void report_execution(int rc, MPI_Comm comm, std::string operation)
{
    if constexpr (BuildOptions::log_level >= LogLevel::errors_and_info)
    {
        int size, rank, len;
        char errstr[MPI_MAX_ERROR_STRING];
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        MPI_Error_string(rc, errstr, &len);
        std::cout << "Rank " << rank << " / " << size << ": " << operation
                  << " done (error: " << errstr << ")" << std::endl;
    }
}
}  // namespace legio