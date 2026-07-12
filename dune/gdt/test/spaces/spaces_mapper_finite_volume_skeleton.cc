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

#include <dune/gdt/spaces/mapper/finite-volume-skeleton.hh>
#include <dune/gdt/spaces/skeleton/finite-volume.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;


// Builds a 2x2 cube grid (the unit square split into 4 quadrilateral elements).
static XT::Grid::GridProvider<G> make_2x2_grid()
{
  return XT::Grid::make_cube_grid<G>(0., 1., 2);
}


// The FiniteVolumeSkeletonMapper attaches exactly one DoF to *every* intersection of the grid, i.e. to every codim-1
// entity (all interior edges plus all boundary edges), see FiniteVolumeSkeletonMapper::size() which returns the size of
// an MCMG mapper over the codim-1 layout.
//
// Hand count for the 2x2 cube grid on the unit square (nodes form a 3x3 lattice):
//   - horizontal edges: 3 grid lines * 2 segments = 6
//   - vertical edges:   3 grid lines * 2 segments = 6
//   => 12 edges total (of which 4 are interior, shared between two elements, and 8 are on the boundary).
// Hence mapper.size() must equal 12, which also equals grid_view.size(1) (number of codim-1 entities).
GTEST_TEST(spaces_mapper_finite_volume_skeleton, size_equals_number_of_intersections)
{
  auto grid = make_2x2_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_skeleton_space(grid_view);

  EXPECT_EQ(12u, space.mapper().size());
  EXPECT_EQ(static_cast<size_t>(grid_view.size(1)), space.mapper().size());
}


GTEST_TEST(spaces_mapper_finite_volume_skeleton, local_size_equals_number_of_element_edges)
{
  auto grid = make_2x2_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_skeleton_space(grid_view);

  size_t observed_max_local_size = 0;
  for (auto&& element : Dune::elements(grid_view)) {
    // subEntities(1) is the number of codim-1 subentities (edges) of the element; a quadrilateral has 4.
    EXPECT_EQ(element.subEntities(1u), space.mapper().local_size(element));
    EXPECT_EQ(4u, space.mapper().local_size(element));
    observed_max_local_size = std::max(observed_max_local_size, space.mapper().local_size(element));
  }
  EXPECT_LE(observed_max_local_size, space.mapper().max_local_size());
  EXPECT_EQ(4u, space.mapper().max_local_size());
}


// Same index-set contract as in spaces_l2_finite_volume.cc (mapper_maps_correctly): collect every global index into a
// std::set, assert the numbering is consecutive (0 .. size()-1), covers the full range and that each index is unique.
GTEST_TEST(spaces_mapper_finite_volume_skeleton, maps_correctly)
{
  auto grid = make_2x2_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_skeleton_space(grid_view);

  std::set<size_t> global_indices;
  std::set<size_t> map_to_global;
  for (auto&& element : Dune::elements(grid_view)) {
    for (auto&& global_index : space.mapper().global_indices(element))
      global_indices.insert(global_index);
    for (size_t ii = 0; ii < space.mapper().local_size(element); ++ii)
      map_to_global.insert(space.mapper().global_index(element, ii));
  }
  // consecutive numbering starting at 0
  EXPECT_EQ(0u, *global_indices.begin());
  EXPECT_EQ(global_indices.size() - 1, *global_indices.rbegin());
  EXPECT_EQ(0u, *map_to_global.begin());
  EXPECT_EQ(map_to_global.size() - 1, *map_to_global.rbegin());
  // the mapper agrees on the size
  EXPECT_EQ(space.mapper().size(), global_indices.size());
  EXPECT_EQ(space.mapper().size(), map_to_global.size());
  // each global index is unique
  for (const auto& global_index : global_indices)
    EXPECT_EQ(1u, global_indices.count(global_index));
  for (const auto& global_index : map_to_global)
    EXPECT_EQ(1u, map_to_global.count(global_index));
}


// The mapper can also be instantiated directly (not only through the space) and is expected to agree with the space.
GTEST_TEST(spaces_mapper_finite_volume_skeleton, standalone_mapper_matches_space)
{
  auto grid = make_2x2_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_skeleton_space(grid_view);
  FiniteVolumeSkeletonMapper<decltype(grid_view)> mapper(grid_view);

  EXPECT_EQ(space.mapper().size(), mapper.size());
  EXPECT_EQ(space.mapper().max_local_size(), mapper.max_local_size());
  for (auto&& element : Dune::elements(grid_view)) {
    ASSERT_EQ(space.mapper().local_size(element), mapper.local_size(element));
    for (size_t ii = 0; ii < mapper.local_size(element); ++ii)
      EXPECT_EQ(space.mapper().global_index(element, ii), mapper.global_index(element, ii));
  }
}
