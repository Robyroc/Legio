#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <string>
#include "config.hpp"
#include "mpi.h"

namespace legio {

constexpr void log(const char* info, const LogLevel level)
{
    if (level <= BuildOptions::log_level)
        std::cout << info << std::endl;
}

void report_execution(int, MPI_Comm, std::string);

}  // namespace legio
#endif