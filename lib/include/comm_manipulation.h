#ifndef COMM_MANIPULATION_H
#define COMM_MANIPULATION_H

#include "mpi.h"
#include <string>

class AdvComm;
class NoComm;
class SingleComm;
class HierarComm;

bool add_comm(MPI_Comm, AdvComm*);

bool add_comm(MPI_Comm, NoComm*);

bool add_comm(MPI_Comm, SingleComm*);

bool add_comm(MPI_Comm, HierarComm*);

void replace_comm(AdvComm*, MPI_Comm);

void replace_comm(AdvComm*, MPI_File);

void replace_comm(AdvComm*, MPI_Win);

void agree_and_eventually_replace(int*, AdvComm*, MPI_Comm);

void agree_and_eventually_replace(int*, AdvComm*, MPI_File);

void agree_and_eventually_replace(int*, AdvComm*, MPI_Win);

void initialization();

void finalization();

void print_info(std::string, MPI_Comm, int);

//void kalive_thread();

//void kill_kalive_thread();

#endif
