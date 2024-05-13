#include <mpi.h>
#include <signal.h>
#include <stdio.h>
#include <shared_mutex>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "context.hpp"
#include "log.hpp"
#include "mpi-ext.h"

extern std::shared_timed_mutex failure_mtx;
using namespace legio;

int MPI_Barrier(MPI_Comm comm)
{
    int rank;
    MPI_Comm_rank(comm, &rank);
    while (1)
    {
        int rc;
        int rank, size;
        bool flag = Context::get().m_comm.part_of(comm);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
            rc = PMPI_Barrier(translated.get_comm());
        }
        else
        {
            rc = PMPI_Barrier(comm);
        }
        failure_mtx.unlock_shared();

        legio::report_execution(rc, comm, "Barrier");
        if (flag)
        {
            agree_and_eventually_replace(&rc, Context::get().m_comm.translate_into_complex(comm));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    while (1)
    {
        int rc;
        bool flag = Context::get().m_comm.part_of(comm);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
            int root_rank = translate_ranks(root, translated);
            if (root_rank == MPI_UNDEFINED)
            {
                if constexpr (BuildOptions::broadcast_resiliency)
                    rc = MPI_SUCCESS;
                else
                {
                    legio::log("##### Broadcast failed, stopping a node", LogLevel::errors_only);
                    raise(SIGINT);
                }
            }
            else
                rc = PMPI_Bcast(buffer, count, datatype, root_rank, translated.get_comm());
        }
        else
            rc = PMPI_Bcast(buffer, count, datatype, root, comm);
        failure_mtx.unlock_shared();
        legio::report_execution(rc, comm, "Bcast");
        if (flag)
        {
            agree_and_eventually_replace(&rc, Context::get().m_comm.translate_into_complex(comm));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_Allreduce(const void* sendbuf,
                  void* recvbuf,
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op,
                  MPI_Comm comm)
{
    while (1)
    {
        int rc;
        bool flag = Context::get().m_comm.part_of(comm);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
            rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, translated.get_comm());
        }
        else
            rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
        failure_mtx.unlock_shared();
        legio::report_execution(rc, comm, "Allreduce");
        if (rc == MPI_SUCCESS || !flag)
            return rc;
        else
            replace_comm(Context::get().m_comm.translate_into_complex(comm));
    }
}

int MPI_Reduce(const void* sendbuf,
               void* recvbuf,
               int count,
               MPI_Datatype datatype,
               MPI_Op op,
               int root,
               MPI_Comm comm)
{
    while (1)
    {
        int rc;
        bool flag = Context::get().m_comm.part_of(comm);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
            int root_rank = translate_ranks(root, translated);
            if (root_rank == MPI_UNDEFINED)
            {
                if constexpr (BuildOptions::reduce_resiliency)
                    rc = MPI_SUCCESS;
                else
                {
                    legio::log("##### Reduce failed, stopping a node", LogLevel::errors_only);
                    raise(SIGINT);
                }
            }
            else
                rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root_rank,
                                 translated.get_comm());
        }
        else
            rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
        failure_mtx.unlock_shared();
        legio::report_execution(rc, comm, "Reduce");
        if (flag)
        {
            agree_and_eventually_replace(&rc, Context::get().m_comm.translate_into_complex(comm));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int perform_gather(const void* sendbuf,
                   int sendcount,
                   MPI_Datatype sendtype,
                   void* recvbuf,
                   int recvcount,
                   MPI_Datatype recvtype,
                   int root,
                   MPI_Comm comm,
                   int totalsize,
                   int fakerank,
                   MPI_Comm fakecomm)
{
    if constexpr (BuildOptions::gather_shift)
        return PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    else
    {
        int type_size, cur_rank;
        MPI_Win win;
        MPI_Type_size(recvtype, &type_size);
        MPI_Comm_rank(comm, &cur_rank);
        if (cur_rank == root)
            MPI_Win_create(recvbuf, totalsize * recvcount * type_size, type_size, MPI_INFO_NULL,
                           fakecomm, &win);
        else
            MPI_Win_create(recvbuf, 0, type_size, MPI_INFO_NULL, fakecomm, &win);
        MPI_Win_fence(0, win);
        MPI_Put(sendbuf, sendcount, sendtype, root, fakerank * recvcount, recvcount, recvtype, win);
        MPI_Win_fence(0, win);
        MPI_Win_free(&win);
        return MPI_SUCCESS;
    }
}

int MPI_Gather(const void* sendbuf,
               int sendcount,
               MPI_Datatype sendtype,
               void* recvbuf,
               int recvcount,
               MPI_Datatype recvtype,
               int root,
               MPI_Comm comm)
{
    while (1)
    {
        int rc, actual_root, total_size, fake_rank;
        bool flag = Context::get().m_comm.part_of(comm);
        MPI_Comm actual_comm;
        MPI_Comm_size(comm, &total_size);
        MPI_Comm_rank(comm, &fake_rank);
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
            actual_comm = translated.get_comm();
            actual_root = translate_ranks(root, translated);
            if (actual_root == MPI_UNDEFINED)
            {
                if constexpr (BuildOptions::gather_resiliency)
                    rc = MPI_SUCCESS;
                else
                {
                    legio::log("##### Gather failed, stopping a node", LogLevel::errors_only);
                    raise(SIGINT);
                }
            }
            else
            {
                failure_mtx.lock_shared();
                rc = perform_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                    actual_root, actual_comm, total_size, fake_rank, comm);
                failure_mtx.unlock_shared();
            }
        }
        else
        {
            actual_comm = comm;
            actual_root = root;
            failure_mtx.lock_shared();
            rc = perform_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                actual_root, actual_comm, total_size, fake_rank, comm);
            failure_mtx.unlock_shared();
        }

        legio::report_execution(rc, comm, "Gather");
        if (flag)
        {
            agree_and_eventually_replace(&rc, Context::get().m_comm.translate_into_complex(comm));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int perform_scatter(const void* sendbuf,
                    int sendcount,
                    MPI_Datatype sendtype,
                    void* recvbuf,
                    int recvcount,
                    MPI_Datatype recvtype,
                    int root,
                    MPI_Comm comm,
                    int totalsize,
                    int fakerank,
                    MPI_Comm fakecomm)
{
    if constexpr (BuildOptions::scatter_shift)
        return PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    else
    {
        int type_size, cur_rank;
        MPI_Win win;
        MPI_Type_size(recvtype, &type_size);
        MPI_Comm_rank(comm, &cur_rank);
        if (cur_rank == root)
            MPI_Win_create((void*)sendbuf, totalsize * sendcount * type_size, type_size,
                           MPI_INFO_NULL, fakecomm, &win);
        else
            MPI_Win_create((void*)sendbuf, 0, type_size, MPI_INFO_NULL, fakecomm, &win);
        MPI_Win_fence(0, win);
        MPI_Get(recvbuf, recvcount, recvtype, root, fakerank * sendcount, sendcount, sendtype, win);
        MPI_Win_fence(0, win);
        MPI_Win_free(&win);
        return MPI_SUCCESS;
    }
}

int MPI_Scatter(const void* sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void* recvbuf,
                int recvcount,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm)
{
    while (1)
    {
        int rc, actual_root, total_size, fake_rank;
        bool flag = Context::get().m_comm.part_of(comm);
        MPI_Comm actual_comm;
        MPI_Comm_size(comm, &total_size);
        MPI_Comm_rank(comm, &fake_rank);
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
            actual_comm = translated.get_comm();
            actual_root = translate_ranks(root, translated);
            if (actual_root == MPI_UNDEFINED)
            {
                if constexpr (BuildOptions::scatter_resiliency)
                    rc = MPI_SUCCESS;
                else
                {
                    legio::log("##### Scatter failed, stopping a node", LogLevel::errors_only);
                    raise(SIGINT);
                }
            }
            else
            {
                failure_mtx.lock_shared();
                rc = perform_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                     actual_root, actual_comm, total_size, fake_rank, comm);
                failure_mtx.unlock_shared();
            }
        }
        else
        {
            actual_comm = comm;
            actual_root = root;
            failure_mtx.lock_shared();
            rc = perform_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                 actual_root, actual_comm, total_size, fake_rank, comm);
            failure_mtx.unlock_shared();
        }

        legio::report_execution(rc, comm, "Scatter");
        if (flag)
        {
            agree_and_eventually_replace(&rc, Context::get().m_comm.translate_into_complex(comm));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_Scan(const void* sendbuf,
             void* recvbuf,
             int count,
             MPI_Datatype datatype,
             MPI_Op op,
             MPI_Comm comm)
{
    while (1)
    {
        int rc;
        bool flag = Context::get().m_comm.part_of(comm);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
            rc = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, translated.get_comm());
        }
        else
            rc = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
        failure_mtx.unlock_shared();
        legio::report_execution(rc, comm, "Scan");
        if (flag)
        {
            agree_and_eventually_replace(&rc, Context::get().m_comm.translate_into_complex(comm));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

// to be checked
int MPI_Allgather(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                       void* recvbuf, int recvcount, MPI_Datatype recvtype,
                       MPI_Comm comm) {

    int rc;
    int size;

    MPI_Comm_size(comm, &size);

    // Gather data from all processes to root process
    rc = MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0, comm);

    // broadcast gathered data from root process to all processes
    rc = MPI_Bcast(recvbuf, recvcount * size, recvtype, 0, comm);

    return rc;
}



