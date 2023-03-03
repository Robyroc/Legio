#include <mpi.h>
#include <signal.h>
#include <stdio.h>
#include <shared_mutex>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "log.hpp"
#include ULFM_HDR
#include "multicomm.hpp"

extern std::shared_timed_mutex failure_mtx;
using namespace legio;

int MPI_Barrier(MPI_Comm comm)
{
    int rank;
    MPI_Comm_rank(comm, &rank);
    while (1)
    {
        int rc;
        Legio_comm com = comm;
        bool flag = Multicomm::get_instance().part_of(com);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(com);
            rc = PMPI_Barrier(translated.get_comm());
        }
        else
        {
            rc = PMPI_Barrier(com);
        }
        failure_mtx.unlock_shared();

        legio::report_execution(rc, com, "Barrier");
        if (flag)
        {
            agree_and_eventually_replace(&rc,
                                         Multicomm::get_instance().translate_into_complex(com));
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
        Legio_comm com = comm;
        bool flag = Multicomm::get_instance().part_of(com);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(com);
            int root_rank = Multicomm::get_instance().translate_ranks(root, translated);
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
            agree_and_eventually_replace(&rc,
                                         Multicomm::get_instance().translate_into_complex(comm));
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
        Legio_comm com = comm;
        bool flag = Multicomm::get_instance().part_of(com);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(com);
            rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, translated.get_comm());
        }
        else
            rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
        failure_mtx.unlock_shared();
        legio::report_execution(rc, comm, "Allreduce");
        if (rc == MPI_SUCCESS || !flag)
            return rc;
        else
            replace_comm(Multicomm::get_instance().translate_into_complex(com));
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
        Legio_comm com = comm;
        bool flag = Multicomm::get_instance().part_of(com);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(com);
            int root_rank = Multicomm::get_instance().translate_ranks(root, translated);
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
            agree_and_eventually_replace(&rc,
                                         Multicomm::get_instance().translate_into_complex(com));
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
        Legio_comm com = comm;
        bool flag = Multicomm::get_instance().part_of(com);
        MPI_Comm actual_comm;
        MPI_Comm_size(comm, &total_size);
        MPI_Comm_rank(comm, &fake_rank);
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(com);
            actual_comm = translated.get_comm();
            actual_root = Multicomm::get_instance().translate_ranks(root, translated);
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
            agree_and_eventually_replace(&rc,
                                         Multicomm::get_instance().translate_into_complex(com));
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
        Legio_comm com = comm;
        bool flag = Multicomm::get_instance().part_of(com);
        MPI_Comm actual_comm;
        MPI_Comm_size(comm, &total_size);
        MPI_Comm_rank(comm, &fake_rank);
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(com);
            actual_comm = translated.get_comm();
            actual_root = Multicomm::get_instance().translate_ranks(root, translated);
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
            agree_and_eventually_replace(&rc,
                                         Multicomm::get_instance().translate_into_complex(com));
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
        Legio_comm com = comm;
        bool flag = Multicomm::get_instance().part_of(com);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(com);
            rc = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, translated.get_comm());
        }
        else
            rc = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
        failure_mtx.unlock_shared();
        legio::report_execution(rc, comm, "Scan");
        if (flag)
        {
            agree_and_eventually_replace(&rc,
                                         Multicomm::get_instance().translate_into_complex(com));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}