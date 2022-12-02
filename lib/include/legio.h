#ifndef LEGIO_H
#define LEGIO_H

#include "mpi.h"

#define LEGIO_MAX_FAILS 50
#define LEGIO_FAILURE_TAG 77
#define LEGIO_PING_TAG 78
#define LEGIO_FAILURE_PING_VALUE 1
#define LEGIO_FAILURE_REPAIR_VALUE 2
#define LEGIO_FAILURE_REPAIR_SELF_VALUE 3

void fault_number(MPI_Comm, int*);

void who_failed(MPI_Comm, int*, int*);

#endif
