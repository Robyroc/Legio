#include "hierar_comm.h"
#include "mpi.h"


HierarComm::HierarComm(MPI_Comm comm): AdvComm(comm) {}

void HierarComm::fault_manage() {}

void HierarComm::destroy(std::function<int(MPI_Comm*)> destroyer) {}

void HierarComm::add_structure(MPI_File file, std::function<int(MPI_Comm, MPI_File*)> adder) {}

void HierarComm::remove_structure(MPI_File file) {}

MPI_File HierarComm::translate_structure(MPI_File file) {return file;}

void HierarComm::check_served(MPI_File file, int* result) {*result = 0;}

int HierarComm::perform_operation(OneToOne op, int rank) {return MPI_SUCCESS;}

int HierarComm::perform_operation(OneToAll op, int rank) {return MPI_SUCCESS;}

int HierarComm::perform_operation(AllToOne op, int rank) {return MPI_SUCCESS;}

int HierarComm::perform_operation(AllToAll op) {return MPI_SUCCESS;}

int HierarComm::perform_operation(FileOp op, MPI_File file) {return MPI_SUCCESS;}

void HierarComm::replace_comm(MPI_Comm new_comm) {}