#ifndef CART_TOPOLOGY_HPP
#define CART_TOPOLOGY_HPP

#include <vector>

namespace legio {

typedef std::vector<int> Coordinate;
typedef std::vector<int> Dimensions;

class Cartesian
{
   public:
    Cartesian(int rank, int ndims, const int* dims, const int* periods);
    Cartesian(int rank, Dimensions dims, std::vector<bool> periods);
    const int get_rank(Coordinate coords) const;
    const int get_rank(const int* coords) const;
    const std::pair<Coordinate, Coordinate> cart_shift(const int dimension,
                                                       const int displacement) const;
    const Coordinate translate(const int rank) const;
    const int get_ndims() const { return dims.size(); }
    const Dimensions& get_dims() const { return dims; }
    const std::vector<bool>& get_periods() const { return periods; }
    const Coordinate& get_coords() const { return cur_coords; }

   private:
    Dimensions dims;
    std::vector<bool> periods;
    Coordinate cur_coords;
};

}  // namespace legio

#endif