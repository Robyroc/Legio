#ifndef LEGIO_H
#define LEGIO_H

#include "mpi.h"

#define LEGIO_MAX_FAILS 50

void fault_number(MPI_Comm, int*);

void who_failed(MPI_Comm, int*, int*);

#endif