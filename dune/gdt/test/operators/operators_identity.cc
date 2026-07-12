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

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/la/container/istl.hh>

#include <dune/gdt/discretefunction/default.hh>
#include <dune/gdt/operators/identity.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using V = XT::LA::IstlDenseVector<double>;


GTEST_TEST(operators_identity, apply_leaves_source_unchanged)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  auto op = make_identity_operator(space);

  V source(n, 0.);
  for (size_t ii = 0; ii < n; ++ii)
    source.set_entry(ii, 1. + double(ii));

  const auto range = op.apply(source);
  ASSERT_EQ(n, range.size());
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(source.get_entry(ii), range.get_entry(ii));
}


GTEST_TEST(operators_identity, apply_inverse_is_identity)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  auto op = make_identity_operator(space);

  V range(n, 0.);
  for (size_t ii = 0; ii < n; ++ii)
    range.set_entry(ii, 3. - double(ii));

  const auto source = op.apply_inverse(range);
  ASSERT_EQ(n, source.size());
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(range.get_entry(ii), source.get_entry(ii));
}


GTEST_TEST(operators_identity, jacobian_is_unit_diagonal)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  auto op = make_identity_operator(space);

  V source(n, 0.);
  auto jac = op.jacobian(source);
  const auto& mat = jac.matrix();
  ASSERT_EQ(n, mat.rows());
  ASSERT_EQ(n, mat.cols());
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(1., mat.get_entry(ii, ii));
}
