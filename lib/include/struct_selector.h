#ifndef STRUCT_SELECTOR_H
#define STRUCT_SELECTOR_H

#include "mpi.h"
#include "structure_handler.h"
#include <tuple>

template<class MPI_T>
int c2f(MPI_T);

template<>
inline int c2f<MPI_Win>(MPI_Win win)
{
    return MPI_Win_c2f(win);
};

template<>
inline int c2f<MPI_File>(MPI_File file)
{
    return MPI_File_c2f(file);
};

template<>
inline int c2f<MPI_Request>(MPI_Request request)
{
    return MPI_Request_c2f(request);
};

typedef std::tuple<
    StructureHandler<MPI_Win, MPI_Comm> *,
    StructureHandler<MPI_File, MPI_Comm> *,
    StructureHandler<MPI_Request, MPI_Comm> *
    > handlers;

template<class MPI_T>
struct handle_selector;

template<>
struct handle_selector<MPI_Win> {
    static constexpr std::size_t get(void) {return 0; }
};

template<>
struct handle_selector<MPI_File> {
    static constexpr std::size_t get(void) {return 1; }
};

template<>
struct handle_selector<MPI_Request> {
    static constexpr std::size_t get(void) {return 2; }
};

#endif