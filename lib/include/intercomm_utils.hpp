#ifndef INTERCOMM_UTILS_HPP
#define INTERCOMM_UTILS_HPP

#include "complex_comm.hpp"
#include "mpi.h"

namespace legio {

int get_root_level(int own_rank, int max_level);
void get_range(int prefix, int level, int size, int* low, int* high);
void check_group(ComplexComm complex_comm,
                 MPI_Group group,
                 MPI_Group* first_clean,
                 MPI_Group* second_clean);
unsigned next_pow_2(int number);
MPI_Group deeper_check_tree(MPI_Group group, MPI_Comm comm);
MPI_Group deeper_check_cube(MPI_Group group, MPI_Comm comm);

}  // namespace legio

#endif