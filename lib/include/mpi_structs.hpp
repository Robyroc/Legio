#ifndef MPI_STRUCTS_HPP
#define MPI_STRUCTS_HPP

#include "mpi.h"

template <class MPI_T>
struct MPI_T_wrapper
{
    constexpr static auto name() -> decltype(MPI_T::name()) { return MPI_T::name(); }
    int value;
    inline MPI_T_wrapper(int a) : value(a){};

    operator int() const { return value; };
    MPI_T_wrapper operator=(MPI_T input) { return MPI_T_wrapper(input); };
};
struct MPI_Comm_
{
    constexpr static const char* name() { return "MPI_Comm"; }
};
struct MPI_Win_
{
    constexpr static const char* name() { return "MPI_Win"; }
};
/*
struct MPI_File_
{
    constexpr static const char* name() { return "MPI_File"; }
};
*/
struct MPI_Request_
{
    constexpr static const char* name() { return "MPI_Request"; }
};
struct MPI_Session_
{
    constexpr static const char* name() { return "MPI_Session"; }
};

#ifdef MPICH
#define MPI_ERR_PROC_FAILED MPIX_ERR_PROC_FAILED
#define ULFM_HDR "mpi_proto.h"
using Legio_comm = MPI_T_wrapper<MPI_Comm_>;
using Legio_win = MPI_T_wrapper<MPI_Win_>;
using Legio_file = MPI_File;
using Legio_request = MPI_T_wrapper<MPI_Request_>;
using Legio_session = MPI_T_wrapper<MPI_Session_>;
#else
#define ULFM_HDR "mpi-ext.h"
using Legio_comm = MPI_Comm;
using Legio_win = MPI_Win;
using Legio_file = MPI_File;
using Legio_request = MPI_Request;
using Legio_session = MPI_Session;
#endif

#endif