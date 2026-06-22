// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include <dune/xt/test/main.hxx>

#include <functional>

#include <dune/common/fvector.hh>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/float_cmp.hh>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/integrals.hh>
#include <dune/xt/grid/type_traits.hh>

using namespace Dune::XT;
using namespace Dune::XT::Grid;

template <class T>
struct IntegralsTest : public ::testing::Test
{
  static constexpr size_t d = T::value;
  using GridType = Dune::YaspGrid<d, Dune::EquidistantOffsetCoordinates<double, d>>;
  using GridViewType = typename GridType::LeafGridView;

  static constexpr size_t num_elements_per_dim = 3;
  const GridProvider<GridType> grid_prv;

  IntegralsTest()
    : grid_prv(make_cube_grid<GridType>(0., 1., num_elements_per_dim))
  {
  }

  // The integral of the constant function 1 over all elements equals the volume of the unit cube domain, which is 1.
  // The integral of a constant c equals c times the same volume.
  void check_element_integral()
  {
    const auto gv = grid_prv.grid().leafGridView();
    double volume = 0.;
    double weighted = 0.;
    const double c = 2.5;
    for (auto&& element : Dune::elements(gv)) {
      volume += element_integral(
          element, std::function<double(const Dune::FieldVector<double, d>&)>([](const auto&) { return 1.; }), 0);
      weighted += element_integral(
          element, std::function<double(const Dune::FieldVector<double, d>&)>([c](const auto&) { return c; }), 0);
    }
    EXPECT_TRUE(Common::FloatCmp::eq(volume, 1.));
    EXPECT_TRUE(Common::FloatCmp::eq(weighted, c));
  }

  // The integral of the constant function 1 over all boundary intersections equals the surface area of the unit cube,
  // which is 2 * d (independent of the refinement level).
  void check_intersection_integral()
  {
    const auto gv = grid_prv.grid().leafGridView();
    double surface = 0.;
    for (auto&& element : Dune::elements(gv)) {
      for (auto&& intersection : Dune::intersections(gv, element)) {
        if (intersection.boundary() && !intersection.neighbor())
          surface += intersection_integral(
              intersection,
              std::function<double(const Dune::FieldVector<double, int(d) - 1>&)>([](const auto&) { return 1.; }),
              0);
      }
    }
    EXPECT_TRUE(Common::FloatCmp::eq(surface, 2. * double(d)));
  }
};

using GridDims = ::testing::Types<::Int<1>, ::Int<2>, ::Int<3>>;
TYPED_TEST_SUITE(IntegralsTest, GridDims);

TYPED_TEST(IntegralsTest, element_integral)
{
  this->check_element_integral();
}
TYPED_TEST(IntegralsTest, intersection_integral)
{
  this->check_intersection_integral();
}
