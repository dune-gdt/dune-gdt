// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-gdt developers

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <algorithm>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/la/container/pattern.hh>

#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/spaces/l2/discontinuous-lagrange.hh>
#include <dune/gdt/tools/sparsity-pattern.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;


static size_t count_entries(const XT::LA::SparsityPatternDefault& pattern)
{
  size_t entries = 0;
  for (size_t ii = 0; ii < pattern.size(); ++ii)
    entries += pattern.inner(ii).size();
  return entries;
}


static bool has_diagonal(const XT::LA::SparsityPatternDefault& pattern)
{
  for (size_t ii = 0; ii < pattern.size(); ++ii) {
    const auto& row = pattern.inner(ii);
    if (std::find(row.begin(), row.end(), ii) == row.end())
      return false;
  }
  return true;
}


GTEST_TEST(sparsity_pattern, element_pattern_continuous_lagrange)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);

  const auto pattern = make_element_sparsity_pattern(space);
  EXPECT_EQ(space.mapper().size(), pattern.size());
  EXPECT_GT(count_entries(pattern), 0u);
  EXPECT_TRUE(has_diagonal(pattern));
}


GTEST_TEST(sparsity_pattern, element_and_intersection_is_superset_of_element)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  // a discontinuous space couples neighbours over intersections
  const auto space = make_discontinuous_lagrange_space(grid_view, 1);

  const auto element = make_element_sparsity_pattern(space);
  const auto element_and_intersection = make_element_and_intersection_sparsity_pattern(space);
  EXPECT_EQ(space.mapper().size(), element.size());
  EXPECT_EQ(space.mapper().size(), element_and_intersection.size());
  // adding the coupling stencil can only introduce additional entries
  EXPECT_GE(count_entries(element_and_intersection), count_entries(element));
}


GTEST_TEST(sparsity_pattern, automatic_stencil_dispatch)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto cg = make_continuous_lagrange_space(grid_view, 1);
  const auto dg = make_discontinuous_lagrange_space(grid_view, 1);

  // continuous spaces use the element stencil ...
  const auto cg_auto = make_sparsity_pattern(cg, Stencil::automatic);
  EXPECT_EQ(count_entries(make_element_sparsity_pattern(cg)), count_entries(cg_auto));

  // ... discontinuous spaces use the element-and-intersection stencil
  const auto dg_auto = make_sparsity_pattern(dg, Stencil::automatic);
  EXPECT_EQ(count_entries(make_element_and_intersection_sparsity_pattern(dg)), count_entries(dg_auto));

  // explicit stencil selection is also exercised
  EXPECT_EQ(count_entries(make_element_sparsity_pattern(cg)),
            count_entries(make_sparsity_pattern(cg, Stencil::element)));
  EXPECT_EQ(count_entries(make_intersection_sparsity_pattern(dg)),
            count_entries(make_sparsity_pattern(dg, Stencil::intersection)));
}
