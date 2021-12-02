#ifndef INTERCOMM_UTILS_H
#define INTERCOMM_UTILS_H

#include "mpi.h"
#include "complex_comm.h"

int get_root_level(int own_rank, int max_level);
void get_range(int prefix, int level, int size, int* low, int* high);
void check_group(ComplexComm complex_comm, MPI_Group group, MPI_Group *first_clean, MPI_Group *second_clean);
MPI_Group deeper_check(MPI_Group group, MPI_Comm comm);

#endif