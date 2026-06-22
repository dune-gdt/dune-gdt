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

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/grid/boundaryinfo.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

using namespace Dune::XT;
using namespace Dune::XT::Grid;

template <class T>
struct BoundaryInfoTest : public ::testing::Test
{
  static constexpr size_t d = T::value;
  using GridType = Dune::YaspGrid<d, Dune::EquidistantOffsetCoordinates<double, d>>;
  using GridViewType = typename GridType::LeafGridView;
  using IntersectionType = extract_intersection_t<GridViewType>;

  const GridProvider<GridType> grid_prv;

  BoundaryInfoTest()
    : grid_prv(make_cube_grid<GridType>(0., 1., 2))
  {
  }

  // For every boundary info, boundary intersections must be classified as the expected boundary type while inner
  // intersections must be classified as no_boundary.
  template <class BoundaryInfoType, class BoundaryType>
  void check_classification(const BoundaryInfoType& info, const BoundaryType& expected_boundary_type)
  {
    const auto gv = grid_prv.grid().leafGridView();
    size_t num_boundary = 0;
    for (auto&& element : Dune::elements(gv)) {
      for (auto&& intersection : Dune::intersections(gv, element)) {
        if (intersection.boundary() && !intersection.neighbor()) {
          EXPECT_EQ(info.type(intersection), expected_boundary_type);
          ++num_boundary;
        } else if (intersection.neighbor()) {
          EXPECT_EQ(info.type(intersection), no_boundary);
        }
      }
    }
    EXPECT_GT(num_boundary, 0);
  }

  void check_alldirichlet()
  {
    const auto info = make_alldirichlet_boundaryinfo(grid_prv.grid().leafGridView());
    check_classification(*info, dirichlet_boundary);
  }

  void check_allneumann()
  {
    const auto info = make_allneumann_boundaryinfo<IntersectionType>();
    check_classification(*info, neumann_boundary);
  }

  void check_allreflecting()
  {
    const auto info = make_allreflecting_boundaryinfo<IntersectionType>();
    check_classification(*info, reflecting_boundary);
  }

  // The default normal-based configuration maps every axis-aligned normal to a Dirichlet boundary, so for an
  // axis-aligned cube grid all boundary intersections must be classified as Dirichlet.
  void check_normalbased()
  {
    const auto info = make_normalbased_boundaryinfo<IntersectionType>(normalbased_boundaryinfo_default_config<d>());
    check_classification(*info, dirichlet_boundary);
    // exercise the textual representation
    EXPECT_FALSE(info->str().empty());
  }
};

using GridDims = ::testing::Types<::Int<1>, ::Int<2>, ::Int<3>>;
TYPED_TEST_SUITE(BoundaryInfoTest, GridDims);

TYPED_TEST(BoundaryInfoTest, alldirichlet)
{
  this->check_alldirichlet();
}
TYPED_TEST(BoundaryInfoTest, allneumann)
{
  this->check_allneumann();
}
TYPED_TEST(BoundaryInfoTest, allreflecting)
{
  this->check_allreflecting();
}
TYPED_TEST(BoundaryInfoTest, normalbased)
{
  this->check_normalbased();
}


GTEST_TEST(BoundaryTypes, make_and_compare)
{
  // note: make_boundary_type does not support AbsorbingBoundary, so it is intentionally omitted here
  for (const auto& id : {NoBoundary().id(),
                         UnknownBoundary().id(),
                         DirichletBoundary().id(),
                         NeumannBoundary().id(),
                         RobinBoundary().id(),
                         ReflectingBoundary().id(),
                         InflowBoundary().id(),
                         OutflowBoundary().id(),
                         InflowOutflowBoundary().id(),
                         ImpermeableBoundary().id()}) {
    std::unique_ptr<BoundaryType> created(make_boundary_type(id));
    EXPECT_EQ(created->id(), id);
    // a copy compares equal to its original ...
    std::unique_ptr<BoundaryType> copied(created->copy());
    EXPECT_EQ(*created, *copied);
    EXPECT_FALSE(*created != *copied);
  }
  // ... while distinct types compare unequal
  EXPECT_NE(DirichletBoundary(), NeumannBoundary());
  EXPECT_THROW(make_boundary_type("not_a_valid_boundary_type"), Exceptions::boundary_type_error);
}
