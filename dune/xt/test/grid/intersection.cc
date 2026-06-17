// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include <dune/xt/test/main.hxx>

#include <cmath>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/logstreams.hh>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/intersection.hh>
#include <dune/xt/grid/type_traits.hh>

using namespace Dune::XT;
using namespace Dune::XT::Common;
using namespace Dune::XT::Grid;

template <class T>
struct IntersectionTest : public ::testing::Test
{
  static constexpr size_t d = T::value;
  using GridType = Dune::YaspGrid<d, Dune::EquidistantOffsetCoordinates<double, d>>;
  using GridViewType = typename GridType::LeafGridView;

  static constexpr size_t num_elements_per_dim = 2;
  const GridProvider<GridType> grid_prv;

  IntersectionTest()
    : grid_prv(make_cube_grid<GridType>(0., 1., num_elements_per_dim))
  {
  }

  // For an axis-aligned cube grid, the center unit outer normal of any intersection has unit length and is
  // axis-aligned (exactly one component is +-1, all others are 0).
  void check_normals()
  {
    const auto gv = grid_prv.grid().leafGridView();
    for (auto&& element : Dune::elements(gv)) {
      for (auto&& intersection : Dune::intersections(gv, element)) {
        const auto normal = intersection.centerUnitOuterNormal();
        EXPECT_TRUE(FloatCmp::eq(normal.two_norm(), 1.));
        size_t non_zero = 0;
        for (size_t ii = 0; ii < d; ++ii)
          if (!FloatCmp::eq(std::abs(normal[ii]), 0.))
            ++non_zero;
        EXPECT_EQ(1u, non_zero);
      }
    }
  }

  // The diameter of a codim-1 face of a cube of edge h is h * sqrt(d - 1) (only meaningful for d >= 2, since in 1d
  // an intersection is a single point).
  void check_diameter()
  {
    if (d < 2)
      return;
    const auto gv = grid_prv.grid().leafGridView();
    const double h = 1. / double(num_elements_per_dim);
    const double expected = h * std::sqrt(double(d - 1));
    for (auto&& element : Dune::elements(gv))
      for (auto&& intersection : Dune::intersections(gv, element))
        EXPECT_TRUE(FloatCmp::eq(diameter(intersection), expected));
  }

  // The center of an intersection lies on the intersection; a point translated away from the center along the outer
  // normal does not.
  void check_contains()
  {
    const auto gv = grid_prv.grid().leafGridView();
    for (auto&& element : Dune::elements(gv)) {
      for (auto&& intersection : Dune::intersections(gv, element)) {
        const auto center = intersection.geometry().center();
        EXPECT_TRUE(contains(intersection, center));
        auto outside = center;
        outside.axpy(2., intersection.centerUnitOuterNormal());
        EXPECT_FALSE(contains(intersection, outside));
      }
    }
  }

  void check_print()
  {
    const auto gv = grid_prv.grid().leafGridView();
    for (auto&& element : Dune::elements(gv)) {
      for (auto&& intersection : Dune::intersections(gv, element)) {
        print_intersection(intersection, "intersection", dev_null);
        print_intersection(intersection, "", dev_null);
        return; // one intersection is enough to exercise the output code path
      }
    }
  }
};

using GridDims = ::testing::Types<Int<1>, Int<2>, Int<3>>;
TYPED_TEST_SUITE(IntersectionTest, GridDims);

TYPED_TEST(IntersectionTest, normals)
{
  this->check_normals();
}
TYPED_TEST(IntersectionTest, diameter)
{
  this->check_diameter();
}
TYPED_TEST(IntersectionTest, contains)
{
  this->check_contains();
}
TYPED_TEST(IntersectionTest, print)
{
  this->check_print();
}
