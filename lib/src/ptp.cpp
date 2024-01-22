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

int any_recv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status*);

int MPI_Send(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int i, rc;
    bool flag = Context::get().m_comm.part_of(comm);
    for (i = 0; i < BuildOptions::num_retry; i++)
    {
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
            int dest_rank = translate_ranks(dest, translated);
            if (dest_rank == MPI_UNDEFINED)
            {
                if constexpr (BuildOptions::send_resiliency)
                    rc = MPI_SUCCESS;
                else
                {
                    legio::log("##### Send failed, stopping a node", LogLevel::errors_only);
                    raise(SIGINT);
                }
            }
            else
                rc = PMPI_Send(buf, count, datatype, dest_rank, tag, translated.get_comm());
        }
        else
            rc = PMPI_Send(buf, count, datatype, dest, tag, comm);
        legio::report_execution(rc, comm, "Send");
        if (rc == MPI_SUCCESS)
            return rc;
    }
    return rc;
}

int MPI_Recv(void* buf,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status* status)
{
    if (source == MPI_ANY_SOURCE)
        return any_recv(buf, count, datatype, source, tag, comm, status);

    int rc;
    bool flag = Context::get().m_comm.part_of(comm);
    ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
    failure_mtx.lock_shared();
    if (flag)
    {
        int source_rank = translate_ranks(source, translated);
        if (source_rank == MPI_UNDEFINED)
        {
            if constexpr (BuildOptions::recv_resiliency)
                rc = MPI_SUCCESS;
            else
            {
                legio::log("##### Recv failed, stopping a node", LogLevel::errors_only);
                raise(SIGINT);
            }
        }
        else
            rc = PMPI_Recv(buf, count, datatype, source_rank, tag, translated.get_comm(), status);
    }
    else
        rc = PMPI_Recv(buf, count, datatype, source, tag, translated.get_comm(), status);
    failure_mtx.unlock_shared();
    legio::report_execution(rc, comm, "Recv");
    return rc;
}

int MPI_Sendrecv(const void* sendbuf,
                 int sendcount,
                 MPI_Datatype sendtype,
                 int dest,
                 int sendtag,
                 void* recvbuf,
                 int recvcount,
                 MPI_Datatype recvtype,
                 int source,
                 int recvtag,
                 MPI_Comm comm,
                 MPI_Status* status)
{
    int rc;
    bool flag = Context::get().m_comm.part_of(comm);
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
        int source_rank = translate_ranks(source, translated);
        int dest_rank = translate_ranks(dest, translated);
        if (source_rank == MPI_UNDEFINED)
        {
            if constexpr (BuildOptions::recv_resiliency && BuildOptions::send_resiliency)
                rc = MPI_SUCCESS;
            else
            {
                legio::log("##### Sendrecv failed, stopping a node", LogLevel::errors_only);
                raise(SIGINT);
            }
        }
        else
            rc = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest_rank, sendtag, recvbuf, recvcount,
                               recvtype, source_rank, recvtag, translated.get_comm(), status);
    }
    else
        rc = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount,
                           recvtype, source, recvtag, comm, status);
    failure_mtx.unlock_shared();
    legio::report_execution(rc, comm, "Sendrecv");
    return rc;
}

int MPI_Sendrecv_replace(void* sendbuf,
                         int count,
                         MPI_Datatype datatype,
                         int dest,
                         int sendtag,
                         int source,
                         int recvtag,
                         MPI_Comm comm,
                         MPI_Status* status)
{
    int rc;
    bool flag = Context::get().m_comm.part_of(comm);
    failure_mtx.lock_shared();
    if (flag)
    {
        ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
        int source_rank = translate_ranks(source, translated);
        int dest_rank = translate_ranks(dest, translated);
        if (source_rank == MPI_UNDEFINED && dest_rank == MPI_UNDEFINED)
        {
            if constexpr (BuildOptions::recv_resiliency && BuildOptions::send_resiliency)
                rc = MPI_SUCCESS;
            else
            {
                legio::log("##### Sendrecv_replace failed, stopping a node", LogLevel::errors_only);
                raise(SIGINT);
            }
        }
        if (source_rank == MPI_UNDEFINED)
        {
            if constexpr (BuildOptions::recv_resiliency)
                rc = PMPI_Send(sendbuf, count, datatype, dest_rank, sendtag, translated.get_comm());
            else
            {
                legio::log("##### Sendrecv_replace failed, stopping a node", LogLevel::errors_only);
                raise(SIGINT);
            }
        }
        else if (dest_rank == MPI_UNDEFINED)
        {
            if constexpr (BuildOptions::send_resiliency)
                rc = PMPI_Recv(sendbuf, count, datatype, source_rank, recvtag,
                               translated.get_comm(), status);
            else
            {
                legio::log("##### Sendrecv_replace failed, stopping a node", LogLevel::errors_only);
                raise(SIGINT);
            }
        }
        else
            rc = PMPI_Sendrecv_replace(sendbuf, count, datatype, dest_rank, sendtag, source_rank,
                                       recvtag, translated.get_comm(), status);
    }
    else
        rc = PMPI_Sendrecv_replace(sendbuf, count, datatype, dest, sendtag, source, recvtag, comm,
                                   status);
    failure_mtx.unlock_shared();
    legio::report_execution(rc, comm, "Sendrecv");
    return rc;
}

int any_recv(void* buf,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status* status)
{
    int rc;
    bool flag = Context::get().m_comm.part_of(comm);
    if (flag)
    {
        ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
        rc = PMPI_Recv(buf, count, datatype, source, tag, translated.get_comm(), status);
    }
    else
        rc = PMPI_Recv(buf, count, datatype, source, tag, comm, status);
    legio::report_execution(rc, comm, "Recv");
    if (rc != MPI_SUCCESS)
    {
        /*
        int eclass;
        MPI_Error_class(rc, &eclass);
        if( MPIX_ERR_PROC_FAILED != eclass )
        {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        */

        MPIX_Comm_failure_ack(Context::get().m_comm.translate_into_complex(comm).get_comm());
    }
    return rc;
}