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
#include <vector>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/la/container/pattern.hh>

#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/spaces/hdiv/raviart-thomas.hh>
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


// returns true if every column of lhs's row also appears in rhs's row, for all rows
static bool is_row_wise_subset(const XT::LA::SparsityPatternDefault& lhs, const XT::LA::SparsityPatternDefault& rhs)
{
  if (lhs.size() != rhs.size())
    return false;
  for (size_t ii = 0; ii < lhs.size(); ++ii) {
    const auto& rhs_row = rhs.inner(ii);
    for (const size_t jj : lhs.inner(ii))
      if (std::find(rhs_row.begin(), rhs_row.end(), jj) == rhs_row.end())
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
  // adding the coupling stencil can only introduce additional entries, so the element pattern has to
  // be contained row-wise in the element-and-intersection pattern
  EXPECT_TRUE(is_row_wise_subset(element, element_and_intersection));
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
  const auto cg_element = make_element_sparsity_pattern(cg);
  EXPECT_TRUE(is_row_wise_subset(cg_element, cg_auto));
  EXPECT_TRUE(is_row_wise_subset(cg_auto, cg_element));

  // ... discontinuous spaces use the element-and-intersection stencil
  const auto dg_auto = make_sparsity_pattern(dg, Stencil::automatic);
  const auto dg_element_and_intersection = make_element_and_intersection_sparsity_pattern(dg);
  EXPECT_TRUE(is_row_wise_subset(dg_element_and_intersection, dg_auto));
  EXPECT_TRUE(is_row_wise_subset(dg_auto, dg_element_and_intersection));

  // explicit stencil selection is also exercised
  EXPECT_EQ(count_entries(make_element_sparsity_pattern(cg)),
            count_entries(make_sparsity_pattern(cg, Stencil::element)));
  EXPECT_EQ(count_entries(make_intersection_sparsity_pattern(dg)),
            count_entries(make_sparsity_pattern(dg, Stencil::intersection)));
}


GTEST_TEST(sparsity_pattern, dispatcher_covers_remaining_branches)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto cg = make_continuous_lagrange_space(grid_view, 1);
  const auto dg = make_discontinuous_lagrange_space(grid_view, 1);
  // a Raviart-Thomas space is discontinuous but has continuous normal components
  const auto rt = make_raviart_thomas_space(grid_view, 0);

  // the explicit element_and_intersection stencil is the one dispatcher branch not hit above
  const auto dg_element_and_intersection = make_element_and_intersection_sparsity_pattern(dg);
  const auto dg_explicit = make_sparsity_pattern(dg, Stencil::element_and_intersection);
  EXPECT_TRUE(is_row_wise_subset(dg_element_and_intersection, dg_explicit));
  EXPECT_TRUE(is_row_wise_subset(dg_explicit, dg_element_and_intersection));

  // automatic dispatch: continuous_normal_components() short-circuits the || chain to the element stencil
  const auto rt_auto = make_sparsity_pattern(rt, Stencil::automatic);
  const auto rt_element = make_element_sparsity_pattern(rt);
  EXPECT_TRUE(is_row_wise_subset(rt_element, rt_auto));
  EXPECT_TRUE(is_row_wise_subset(rt_auto, rt_element));

  // distinct test/ansatz spaces exercise the ansatz-side operands of the || short-circuit chain; both dispatch to the
  // element stencil, which we check for exactly (the row count alone cannot distinguish the stencils, as it is the
  // test-space mapper size either way):
  //   ansatz.continuous(0) (a continuous-Lagrange ansatz) ...
  const auto mixed_cg_ansatz = make_sparsity_pattern(dg, cg, grid_view, Stencil::automatic);
  const auto mixed_cg_element = make_element_sparsity_pattern(dg, cg, grid_view);
  EXPECT_TRUE(is_row_wise_subset(mixed_cg_element, mixed_cg_ansatz));
  EXPECT_TRUE(is_row_wise_subset(mixed_cg_ansatz, mixed_cg_element));
  //   ... and ansatz.continuous_normal_components() (a Raviart-Thomas ansatz), with all earlier operands false
  const auto mixed_rt_ansatz = make_sparsity_pattern(dg, rt, grid_view, Stencil::automatic);
  const auto mixed_rt_element = make_element_sparsity_pattern(dg, rt, grid_view);
  EXPECT_TRUE(is_row_wise_subset(mixed_rt_element, mixed_rt_ansatz));
  EXPECT_TRUE(is_row_wise_subset(mixed_rt_ansatz, mixed_rt_element));

  // an unknown stencil value falls through to the terminal else and throws
  EXPECT_THROW(make_sparsity_pattern(dg, static_cast<Stencil>(99)), XT::Common::Exceptions::wrong_input_given);
}
