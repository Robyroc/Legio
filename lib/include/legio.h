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
// input is a communicator
// output is an integer that indicates how many faults have happened

void who_failed(MPI_Comm, int*, int*);
// input is a comm.
// ypu should first call fault_number to know how much space to allocate for the second int*
// which is a buffer to hold the rank of all the failed processes
// the first int* is the same as the previous function
// which is basically the number of failed processes

int MPIX_Comm_agree_group(MPI_Comm, MPI_Group, int*);

int MPIX_Horizon_from_group(MPI_Group);

#endif
