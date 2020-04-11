#ifndef COMM_MANIPULATION_H
#define COMM_MANIPULATION_H

#include "mpi.h"

void translate_ranks(int, MPI_Comm, int*);

void replace_comm(MPI_Comm*);

void agree_and_eventually_replace(int*, MPI_Comm*);

#endif
