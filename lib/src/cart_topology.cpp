#include "cart_topology.hpp"
#include <mpi.h>
#include <cstdlib>
#include <cstring>

using namespace legio;

Coordinate from_rank_to_coord(const int rank, const Dimensions dims)
{
    int temp_rank = rank;
    std::vector<int> output(dims.size(), 0);
    for (int i = dims.size() - 1; i >= 0; i--)
    {
        output[i] = temp_rank % dims[i];
        temp_rank /= dims[i];
    }
    return output;
}

const int from_coord_to_rank(const Coordinate coord, const Dimensions dims)
{
    int temp_rank = 0;
    int mult_factor = 1;
    for (int i = dims.size() - 1; i >= 0; i--)
    {
        if (coord[i] < 0)
            return MPI_PROC_NULL;
        temp_rank += coord[i] * mult_factor;
        mult_factor *= dims[i];
    }
    return temp_rank;
}

Cartesian::Cartesian(int rank, int _ndims, const int* _dims, const int* _periods)
{
    for (int i = 0; i < _ndims; i++)
    {
        dims.push_back(_dims[i]);
        periods.push_back(_periods[i]);
    }
    cur_coords = from_rank_to_coord(rank, dims);
}

Cartesian::Cartesian(int rank, Dimensions _dims, std::vector<bool> _periods)
    : dims(_dims), periods(_periods)
{
    cur_coords = from_rank_to_coord(rank, _dims);
}

const int Cartesian::get_rank(Coordinate coords) const
{
    return from_coord_to_rank(coords, dims);
}

const int Cartesian::get_rank(const int* coords) const
{
    return get_rank(Coordinate(coords, coords + dims.size()));
}

const Coordinate Cartesian::translate(const int rank) const
{
    return from_rank_to_coord(rank, dims);
}

const std::pair<Coordinate, Coordinate> Cartesian::cart_shift(const int dimension,
                                                              const int displacement) const
{
    Coordinate source;
    Coordinate dest;
    for (int i = 0; i < dims.size(); i++)
    {
        if (i != dimension)
        {
            source.push_back(cur_coords[i]);
            dest.push_back(cur_coords[i]);
        }
        else
        {
            int source_temp = cur_coords[i] - displacement;
            int dest_temp = cur_coords[i] + displacement;
            if (periods[i])
            {
                source_temp %= dims[i];
                dest_temp %= dims[i];
                source.push_back(source_temp < 0 ? source_temp + dims[i] : source_temp);
                dest.push_back(dest_temp < 0 ? dest_temp + dims[i] : dest_temp);
            }
            else
            {
                source.push_back((source_temp < 0 || source_temp >= dims[i]) ? -1 : source_temp);
                dest.push_back((dest_temp < 0 || dest_temp >= dims[i]) ? -1 : dest_temp);
            }
        }
    }
    return std::make_pair(source, dest);
}
