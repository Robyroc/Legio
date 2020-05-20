#ifndef COMM_MANIPULATION_H
#define COMM_MANIPULATION_H

class ComplexComm;

void translate_ranks(int, ComplexComm*, int*);

void replace_comm(ComplexComm*);

void agree_and_eventually_replace(int*, ComplexComm*);

int MPI_Barrier(ComplexComm* comm);

#endif