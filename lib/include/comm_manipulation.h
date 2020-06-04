#ifndef COMM_MANIPULATION_H
#define COMM_MANIPULATION_H

#include "mpi.h"
#include <string>

class AdvComm;

bool add_comm(MPI_Comm);

void translate_ranks(int, AdvComm*, int*);

void replace_comm(AdvComm*);

void agree_and_eventually_replace(int*, AdvComm*);

void initialization();

void finalization();

void print_info(std::string, MPI_Comm, int);

//void kalive_thread();

//void kill_kalive_thread();

#endif
