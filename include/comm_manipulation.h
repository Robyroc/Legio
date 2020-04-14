#ifndef COMM_MANIPULATION_H
#define COMM_MANIPULATION_H

#include "mpi.h"
#include "complex_comm.h"

void translate_ranks(int, MPI_Comm, int*);

void replace_comm(ComplexComm*);

void agree_and_eventually_replace(int*, ComplexComm*);

#endif
