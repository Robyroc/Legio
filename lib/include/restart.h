#ifndef RESTART_H
#define RESTART_H

#include "mpi.h"
#include <thread>
#include <mutex>
#include <condition_variable>


void initialize_comm(int n, const int *ranks, MPI_Comm *newcomm);
void loop_repair_failures();
void repair_failure();
void restart(int);
bool is_respawned();
// TODO(low-priority) Add callback for restart

#endif