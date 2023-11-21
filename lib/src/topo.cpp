#include <mpi.h>
#include <signal.h>
#include <cstring>
#include <shared_mutex>
#include <thread>
#include "cart_topology.hpp"
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "context.hpp"
#include "intercomm_utils.hpp"
#include "log.hpp"
#include "mpi-ext.h"
#include "restart_routines.hpp"
extern "C" {
#include "legio.h"
}

extern std::shared_timed_mutex failure_mtx;
using namespace legio;

int MPI_Cart_create(MPI_Comm old_comm,
                    int ndims,
                    const int dims[],
                    const int periods[],
                    int reorder,
                    MPI_Comm* comm_cart)
{
    while (1)
    {
        int rc, rank, max_dims;
        bool flag = Context::get().m_comm.part_of(old_comm);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(old_comm);
            max_dims = 1;
            for (int i = 0; i < ndims; i++)
                max_dims *= dims[i];
            MPI_Comm_rank(old_comm, &rank);
            rc = PMPI_Comm_split(translated.get_comm(), rank < max_dims, 0, comm_cart);
        }
        else
            rc = PMPI_Cart_create(old_comm, ndims, dims, periods, reorder, comm_cart);
        failure_mtx.unlock_shared();
        legio::report_execution(rc, old_comm, "Cart_create");
        if (flag)
        {
            agree_and_eventually_replace(&rc,
                                         Context::get().m_comm.translate_into_complex(old_comm));
            if (rc == MPI_SUCCESS)
            {
                if (rank >= max_dims)
                {
                    PMPI_Comm_free(comm_cart);
                    *comm_cart = MPI_COMM_NULL;
                }
                else
                {
                    MPI_Comm_set_errhandler(*comm_cart, MPI_ERRORS_RETURN);
                    bool result = Context::get().m_comm.add_comm(*comm_cart);
                    if (result)
                    {
                        ComplexComm& new_complex =
                            Context::get().m_comm.translate_into_complex(*comm_cart);
                        new_complex.set_topology(Cartesian(rank, ndims, dims, periods));
                        return rc;
                    }
                }
            }
        }
        else
            return rc;
    }
}

int MPI_Topo_test(MPI_Comm comm, int* status)
{
    bool flag = Context::get().m_comm.part_of(comm);
    if (flag)
    {
        ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
        if (translated.get_topology())
            *status = MPI_CART;
        // TODO: adjust to support other topologies
        else
            *status = MPI_UNDEFINED;
        return MPI_SUCCESS;
    }
    else
        return PMPI_Topo_test(comm, status);
}

int MPI_Cartdim_get(MPI_Comm comm, int* ndims)
{
    bool flag = Context::get().m_comm.part_of(comm);
    if (flag)
    {
        ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
        std::optional<Cartesian> topo = translated.get_topology();
        // TODO: adjust to support other topologies
        if (!topo)
            return MPI_ERR_TOPOLOGY;
        else
        {
            *ndims = topo.value().get_ndims();
            return MPI_SUCCESS;
        }
    }
    else
        return PMPI_Cartdim_get(comm, ndims);
}

int MPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[])
{
    bool flag = Context::get().m_comm.part_of(comm);
    if (flag)
    {
        ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
        std::optional<Cartesian> topo = translated.get_topology();
        // TODO: adjust to support other topologies
        if (!topo)
            return MPI_ERR_TOPOLOGY;
        else
        {
            int cur_dims = topo.value().get_ndims();
            for (int i = 0; i < maxdims && i < cur_dims; i++)
            {
                dims[i] = topo.value().get_dims()[i];
                periods[i] = topo.value().get_periods()[i];
                coords[i] = topo.value().get_coords()[i];
            }
            return MPI_SUCCESS;
        }
    }
    else
        return PMPI_Cart_get(comm, maxdims, dims, periods, coords);
}

int MPI_Cart_rank(MPI_Comm comm, const int coords[], int* rank)
{
    bool flag = Context::get().m_comm.part_of(comm);
    if (flag)
    {
        ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
        std::optional<Cartesian> topo = translated.get_topology();
        // TODO: adjust to support other topologies
        if (!topo)
            return MPI_ERR_TOPOLOGY;
        else
        {
            *rank = topo.value().get_rank(coords);
            return MPI_SUCCESS;
        }
    }
    else
        return PMPI_Cart_rank(comm, coords, rank);
}

int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[])
{
    bool flag = Context::get().m_comm.part_of(comm);
    if (flag)
    {
        ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
        std::optional<Cartesian> topo = translated.get_topology();
        // TODO: adjust to support other topologies
        if (!topo)
            return MPI_ERR_TOPOLOGY;
        else
        {
            const Coordinate result = topo.value().translate(rank);
            memcpy(coords, result.data(),
                   (maxdims < result.size() ? maxdims : result.size()) * sizeof(int));
            return MPI_SUCCESS;
        }
    }
    else
        return PMPI_Cart_coords(comm, rank, maxdims, coords);
}

int MPI_Cart_shift(MPI_Comm comm, int direction, int disp, int* rank_source, int* rank_dest)
{
    bool flag = Context::get().m_comm.part_of(comm);
    if (flag)
    {
        ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
        std::optional<Cartesian> topo = translated.get_topology();
        // TODO: adjust to support other topologies
        if (!topo)
            return MPI_ERR_TOPOLOGY;
        else
        {
            std::pair<Coordinate, Coordinate> result = topo.value().cart_shift(direction, disp);
            *rank_source = topo.value().get_rank(result.first);
            *rank_dest = topo.value().get_rank(result.second);
            if constexpr (!BuildOptions::cartesian_bridge)
            {
                return MPI_SUCCESS;
            }
            else
            {
                int num_failed;
                fault_number(comm, &num_failed);
                if (num_failed == 0)
                    return MPI_SUCCESS;
                std::vector<int> failed_ranks(num_failed, 0);
                who_failed(comm, &num_failed, failed_ranks.data());
                int cur_disp = disp;
                bool to_check = false;
                do
                {
                    to_check = false;
                    for (auto rank : failed_ranks)
                    {
                        if (rank == *rank_source)
                        {
                            cur_disp += disp;
                            *rank_source = topo.value().get_rank(
                                topo.value().cart_shift(direction, cur_disp).first);
                            to_check = true;
                        }
                    }
                } while (to_check);

                cur_disp = disp;
                do
                {
                    to_check = false;
                    for (auto rank : failed_ranks)
                    {
                        if (rank == *rank_dest)
                        {
                            cur_disp += disp;
                            *rank_dest = topo.value().get_rank(
                                topo.value().cart_shift(direction, cur_disp).second);
                            to_check = true;
                        }
                    }
                } while (to_check);
                return MPI_SUCCESS;
            }
        }
    }
    else
        return PMPI_Cart_shift(comm, direction, disp, rank_source, rank_dest);
}

int MPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm* new_comm)
{
    while (1)
    {
        int rc, rank;
        Dimensions temp_dim;
        std::vector<bool> temp_periods;
        MPI_Comm_rank(comm, &rank);
        bool flag = Context::get().m_comm.part_of(comm);
        failure_mtx.lock_shared();
        if (flag)
        {
            ComplexComm& translated = Context::get().m_comm.translate_into_complex(comm);
            std::optional<Cartesian> topo = translated.get_topology();
            // TODO: adjust to support other topologies
            if (!topo)
                rc = MPI_ERR_TOPOLOGY;
            else
            {
                int temp_color = 0;
                int mult_factor = 1;
                for (int i = 0; i < topo.value().get_ndims(); i++)
                {
                    if (remain_dims[i])
                    {
                        temp_dim.push_back(topo.value().get_dims()[i]);
                        temp_periods.push_back(topo.value().get_periods()[i]);
                    }
                    else
                        temp_color += topo.value().get_coords()[i] * mult_factor;
                    mult_factor *= topo.value().get_dims()[i];
                }
                rc = PMPI_Comm_split(comm, temp_color, 0, new_comm);
            }
        }
        else
            rc = PMPI_Cart_sub(comm, remain_dims, new_comm);
        failure_mtx.unlock_shared();
        legio::report_execution(rc, comm, "Cart_sub");
        if (flag && rc != MPI_ERR_TOPOLOGY)
        {
            agree_and_eventually_replace(&rc, Context::get().m_comm.translate_into_complex(comm));
            if (rc == MPI_SUCCESS)
            {
                MPI_Comm_set_errhandler(*new_comm, MPI_ERRORS_RETURN);
                bool result = Context::get().m_comm.add_comm(*new_comm);
                if (result)
                {
                    ComplexComm& new_complex =
                        Context::get().m_comm.translate_into_complex(*new_comm);
                    new_complex.set_topology(Cartesian(rank, temp_dim, temp_periods));
                    return rc;
                }
            }
        }
        else
            return rc;
    }
}
