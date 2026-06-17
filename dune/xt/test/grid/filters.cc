// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include <dune/xt/test/main.hxx>

#include <memory>

#include <dune/grid/common/partitionset.hh>
#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/grid/boundaryinfo.hh>
#include <dune/xt/grid/filters.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

using namespace Dune::XT;
using namespace Dune::XT::Grid;

template <class T>
struct FilterTest : public ::testing::Test
{
  static constexpr size_t d = T::value;
  using GridType = Dune::YaspGrid<d, Dune::EquidistantOffsetCoordinates<double, d>>;
  using GridViewType = typename GridType::LeafGridView;

  // use four cells per dimension so that (for d >= 2) interior elements without boundary intersections exist
  static constexpr size_t num_elements_per_dim = 4;
  const GridProvider<GridType> grid_prv;

  FilterTest()
    : grid_prv(make_cube_grid<GridType>(0., 1., num_elements_per_dim))
  {
  }

  template <class Filter, class Range>
  size_t count_elements(const Filter& filter, const GridViewType& gv, const Range& range)
  {
    size_t count = 0;
    for (auto&& element : range)
      if (filter.contains(gv, element))
        ++count;
    return count;
  }

  void check_element_filters()
  {
    const auto gv = grid_prv.grid().leafGridView();
    const size_t num_elements = gv.size(0);

    // count expected values by hand
    size_t expected_boundary = 0;
    for (auto&& element : Dune::elements(gv))
      if (element.hasBoundaryIntersections())
        ++expected_boundary;

    EXPECT_EQ(num_elements, count_elements(ApplyOn::AllElements<GridViewType>(), gv, Dune::elements(gv)));
    EXPECT_EQ(0u, count_elements(ApplyOn::NoElements<GridViewType>(), gv, Dune::elements(gv)));
    EXPECT_EQ(expected_boundary, count_elements(ApplyOn::BoundaryElements<GridViewType>(), gv, Dune::elements(gv)));
    EXPECT_GT(expected_boundary, 0);
    EXPECT_EQ(
        num_elements,
        count_elements(ApplyOn::PartitionSetElements<GridViewType, Dune::Partitions::All>(), gv, Dune::elements(gv)));

    // generic filter: select elements with index < num_elements / 2
    ApplyOn::GenericFilteredElements<GridViewType> half([](const auto& grid_view, const auto& element) {
      return grid_view.indexSet().index(element) < grid_view.indexSet().size(0) / 2;
    });
    EXPECT_EQ(num_elements / 2, count_elements(half, gv, Dune::elements(gv)));

    // copy() must yield an equivalent filter
    std::unique_ptr<ElementFilter<GridViewType>> copied(ApplyOn::AllElements<GridViewType>().copy());
    EXPECT_EQ(num_elements, count_elements(*copied, gv, Dune::elements(gv)));
  }

  template <class Filter>
  size_t count_intersections(const Filter& filter, const GridViewType& gv)
  {
    size_t count = 0;
    for (auto&& element : Dune::elements(gv))
      for (auto&& intersection : Dune::intersections(gv, element))
        if (filter.contains(gv, intersection))
          ++count;
    return count;
  }

  void check_intersection_filters()
  {
    const auto gv = grid_prv.grid().leafGridView();

    // count expected values by hand
    size_t total = 0, boundary = 0, inner = 0;
    for (auto&& element : Dune::elements(gv)) {
      for (auto&& intersection : Dune::intersections(gv, element)) {
        ++total;
        if (intersection.boundary())
          ++boundary;
        if (intersection.neighbor() && !intersection.boundary())
          ++inner;
      }
    }
    EXPECT_EQ(total, num_elements_per_dim_pow_d() * 2 * d);
    EXPECT_EQ(total, boundary + inner);

    EXPECT_EQ(total, count_intersections(ApplyOn::AllIntersections<GridViewType>(), gv));
    EXPECT_EQ(0u, count_intersections(ApplyOn::NoIntersections<GridViewType>(), gv));
    EXPECT_EQ(boundary, count_intersections(ApplyOn::BoundaryIntersections<GridViewType>(), gv));
    EXPECT_EQ(boundary, count_intersections(ApplyOn::NonPeriodicBoundaryIntersections<GridViewType>(), gv));
    EXPECT_EQ(inner, count_intersections(ApplyOn::InnerIntersections<GridViewType>(), gv));

    // a generic filter selecting boundary intersections must agree with BoundaryIntersections
    ApplyOn::GenericFilteredIntersections<GridViewType> generic_boundary(
        [](const auto&, const auto& intersection) { return intersection.boundary(); });
    EXPECT_EQ(boundary, count_intersections(generic_boundary, gv));

    // CustomBoundaryIntersections combined with an all-Dirichlet boundary info selects every boundary intersection
    const auto info = make_alldirichlet_boundaryinfo(gv);
    ApplyOn::CustomBoundaryIntersections<GridViewType> custom(*info, new DirichletBoundary());
    EXPECT_EQ(boundary, count_intersections(custom, gv));

    // copy() must yield an equivalent filter
    std::unique_ptr<IntersectionFilter<GridViewType>> copied(ApplyOn::BoundaryIntersections<GridViewType>().copy());
    EXPECT_EQ(boundary, count_intersections(*copied, gv));
  }

  static size_t num_elements_per_dim_pow_d()
  {
    size_t result = 1;
    for (size_t ii = 0; ii < d; ++ii)
      result *= num_elements_per_dim;
    return result;
  }
};

using GridDims = ::testing::Types<::Int<1>, ::Int<2>, ::Int<3>>;
TYPED_TEST_SUITE(FilterTest, GridDims);

TYPED_TEST(FilterTest, element_filters)
{
  this->check_element_filters();
}
TYPED_TEST(FilterTest, intersection_filters)
{
  this->check_intersection_filters();
}
