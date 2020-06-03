#ifndef COMM_MANIPULATION_H
#define COMM_MANIPULATION_H

#include "mpi.h"

class AdvComm;

bool add_comm(MPI_Comm);

void translate_ranks(int, AdvComm*, int*);

void replace_comm(AdvComm*);

void agree_and_eventually_replace(int*, AdvComm*);

void initialization();

void finalization();

//void kalive_thread();

//void kill_kalive_thread();

#endif
