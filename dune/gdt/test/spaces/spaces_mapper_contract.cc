// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2018)
//   René Fritze     (2018)

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <algorithm>
#include <set>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>

#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/spaces/l2/discontinuous-lagrange.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;


// Applies the mapper index-set contract from spaces/base.hh (mapper_(of_discontinuous_space_)maps_correctly): collect
// every global index reported by both call variants into a std::set, then assert the global numbering is consecutive
// (0 .. size()-1), covers the whole range and that mapper.size() agrees. This holds for continuous mappers (which share
// DoFs across elements, so the union of local index sets is smaller than the sum of local sizes but still spans
// {0, ..., size()-1}) as well as for discontinuous / finite-volume mappers (unique block of DoFs per element).
template <class SpaceType>
static void check_mapper_maps_correctly(const SpaceType& space)
{
  std::set<size_t> global_indices;
  std::set<size_t> map_to_global;
  for (auto&& element : Dune::elements(space.grid_view())) {
    for (auto&& global_index : space.mapper().global_indices(element))
      global_indices.insert(global_index);
    for (size_t ii = 0; ii < space.mapper().local_size(element); ++ii)
      map_to_global.insert(space.mapper().global_index(element, ii));
  }
  ASSERT_GT(global_indices.size(), 0u);
  EXPECT_EQ(0u, *global_indices.begin());
  EXPECT_EQ(global_indices.size() - 1, *global_indices.rbegin());
  EXPECT_EQ(0u, *map_to_global.begin());
  EXPECT_EQ(map_to_global.size() - 1, *map_to_global.rbegin());
  EXPECT_EQ(space.mapper().size(), global_indices.size());
  EXPECT_EQ(space.mapper().size(), map_to_global.size());
  for (const auto& global_index : global_indices)
    EXPECT_EQ(1u, global_indices.count(global_index));
  for (const auto& global_index : map_to_global)
    EXPECT_EQ(1u, map_to_global.count(global_index));
}


GTEST_TEST(spaces_mapper_contract, continuous_lagrange_maps_correctly)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  check_mapper_maps_correctly(space);
}


GTEST_TEST(spaces_mapper_contract, discontinuous_lagrange_maps_correctly)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_discontinuous_lagrange_space(grid_view, 1);
  check_mapper_maps_correctly(space);
}


GTEST_TEST(spaces_mapper_contract, finite_volume_maps_correctly)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  check_mapper_maps_correctly(space);
}


// The continuous mapper shares DoFs between neighbouring elements, hence its global size has to be strictly smaller
// than the discontinuous mapper's size on the same grid (which gives every element its own private block of DoFs).
GTEST_TEST(spaces_mapper_contract, continuous_shares_dofs_discontinuous_does_not)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto cg = make_continuous_lagrange_space(grid_view, 1);
  const auto dg = make_discontinuous_lagrange_space(grid_view, 1);
  const auto fv = make_finite_volume_space(grid_view);

  EXPECT_LT(cg.mapper().size(), dg.mapper().size());
  // one DoF per element for FV, Q1 has 4 local DoFs per element for DG
  EXPECT_EQ(fv.mapper().size() * 4u, dg.mapper().size());
}
