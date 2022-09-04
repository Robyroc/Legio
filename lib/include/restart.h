#ifndef RESTART_H
#define RESTART_H

#include "mpi.h"
#include <thread>
#include <mutex>
#include <condition_variable>


void initialize_comm(MPI_Comm, MPI_Group, int, MPI_Comm*);
void loop_repair_failures();
void repair_failure();
void restart(int);
bool is_respawned();

#endif