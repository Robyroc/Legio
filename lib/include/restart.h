#ifndef RESTART_H
#define RESTART_H

#include "mpi.h"

void initialize_comm(const int n, const int* ranks, MPI_Comm* newcomm);
int is_respawned();
void add_critical(int rank);
// TODO(low-priority) Add callback for restart

#endif