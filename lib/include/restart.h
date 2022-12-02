#ifndef RESTART_H
#define RESTART_H

#include "mpi.h"

void initialize_comm(const int n, const int* ranks, MPI_Comm* newcomm);
void loop_repair_failures();
void repair_failure();
void restart(int);
int is_respawned();
// TODO(low-priority) Add callback for restart

#endif