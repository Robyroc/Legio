#ifndef COMM_MANIPULATION_HPP
#define COMM_MANIPULATION_HPP

namespace legio {

class ComplexComm;

int translate_ranks(int, ComplexComm&);

void replace_comm(ComplexComm&);

void replace_and_repair_comm(ComplexComm& cur_complex);

void agree_and_eventually_replace(int*, ComplexComm&);

void initialization(int* argc, char*** argv);

void finalization();

}  // namespace legio

#endif
