#ifndef STRUCT_SELECTOR_HPP
#define STRUCT_SELECTOR_HPP

#include <tuple>
#include "mpi.h"
#include "structure_handler.hpp"

using namespace legio;

template <class MPI_T>
const int c2f(const MPI_T);

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

typedef std::tuple<StructureHandler<MPI_Win, MPI_Comm>*,
                   StructureHandler<MPI_File, MPI_Comm>*,
                   StructureHandler<MPI_Request, MPI_Comm>*>
    handlers;

template <class MPI_T>
struct handle_selector;

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

#endif