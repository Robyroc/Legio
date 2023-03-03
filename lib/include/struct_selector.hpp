#ifndef STRUCT_SELECTOR_HPP
#define STRUCT_SELECTOR_HPP

#include <tuple>
#include "mpi.h"
#include "mpi_structs.hpp"
#include "structure_handler.hpp"

using namespace legio;

template <class MPI_T>
const int c2f(const MPI_T);

/*
template <>
inline const int c2f<MPI_Win>(const MPI_Win win)
{
    return MPI_Win_c2f(const_cast<MPI_Win>(win));
};

template <>
inline const int c2f<MPI_File>(const MPI_File file)
{
    return MPI_File_c2f(const_cast<MPI_File>(file));
};

template <>
inline const int c2f<MPI_Request>(const MPI_Request request)
{
    return MPI_Request_c2f(const_cast<MPI_Request>(request));
};

template <>
inline const int c2f<MPI_Comm>(const MPI_Comm comm)
{
    return MPI_Comm_c2f(const_cast<MPI_Comm>(comm));
}

template <>
inline const int c2f<MPI_Session>(const MPI_Session session)
{
    return MPI_Session_c2f(const_cast<MPI_Session>(session));
}
*/

template <>
inline const int c2f<Legio_win>(const Legio_win win)
{
    return MPI_Win_c2f(win);
};

template <>
inline const int c2f<Legio_file>(const Legio_file file)
{
    return MPI_File_c2f(file);
};

template <>
inline const int c2f<Legio_request>(const Legio_request request)
{
    return MPI_Request_c2f(request);
};

template <>
inline const int c2f<Legio_comm>(const Legio_comm comm)
{
    return MPI_Comm_c2f(comm);
}

template <>
inline const int c2f<Legio_session>(const Legio_session session)
{
    return MPI_Session_c2f(session);
}

/*

typedef std::tuple<StructureHandler<MPI_Win, MPI_Comm>*,
                   StructureHandler<MPI_File, MPI_Comm>*,
                   StructureHandler<MPI_Request, MPI_Comm>*>
    handlers;

*/

typedef std::tuple<StructureHandler<Legio_win, MPI_Comm>*,
                   StructureHandler<Legio_file, MPI_Comm>*,
                   StructureHandler<Legio_request, MPI_Comm>*>
    handlers;

template <class MPI_T>
struct handle_selector;

/*

template <>
struct handle_selector<MPI_Win>
{
    static constexpr std::size_t get(void) { return 0; }
};

template <>
struct handle_selector<MPI_File>
{
    static constexpr std::size_t get(void) { return 1; }
};

template <>
struct handle_selector<MPI_Request>
{
    static constexpr std::size_t get(void) { return 2; }
};

*/

template <>
struct handle_selector<Legio_win>
{
    static constexpr std::size_t get(void) { return 0; }
};

template <>
struct handle_selector<Legio_file>
{
    static constexpr std::size_t get(void) { return 1; }
};

template <>
struct handle_selector<Legio_request>
{
    static constexpr std::size_t get(void) { return 2; }
};

#endif