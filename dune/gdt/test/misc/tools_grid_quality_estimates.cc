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

#include <cmath>

#include <dune/xt/common/lapacke.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>

#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/tools/grid-quality-estimates.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;


GTEST_TEST(grid_quality_estimates, element_to_intersection_equivalence_constant_on_uniform_cube)
{
  // 4 x 4 uniform partition of the unit square, so each element is a square of side h = 1 / 4.
  auto grid = XT::Grid::make_cube_grid<G>(/*lower_left=*/0., /*upper_right=*/1., /*num_elements=*/4);
  auto grid_view = grid.leaf_view();

  // XT::Grid::diameter() returns the largest distance between any two corners:
  //   element diameter      = h * sqrt(2) (the diagonal of the square)
  //   intersection diameter = h           (the length of an edge)
  // so the (constant over the uniform grid) ratio intersection_diameter / element_diameter is 1 / sqrt(2).
  const auto result = estimate_element_to_intersection_equivalence_constant(grid_view);
  EXPECT_NEAR(1. / std::sqrt(2.), result, 1e-13);
}


GTEST_TEST(grid_quality_estimates, inverse_inequality_constants_are_positive)
{
  auto grid = XT::Grid::make_cube_grid<G>(/*lower_left=*/0., /*upper_right=*/1., /*num_elements=*/2);
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);

  // both estimates rely on a generalized eigen-solver and throw dependency_missing without lapacke
  if (XT::Common::Lapacke::available()) {
    EXPECT_GT(estimate_inverse_inequality_constant(space), 0.);
    EXPECT_GT(estimate_combined_inverse_trace_inequality_constant(space), 0.);
  }
}
