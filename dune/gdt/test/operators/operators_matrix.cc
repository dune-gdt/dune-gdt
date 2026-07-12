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

#include <dune/gdt/operators/matrix.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/tools/sparsity-pattern.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using M = XT::LA::IstlRowMajorSparseMatrix<double>;
using V = XT::LA::IstlDenseVector<double>;


// well-conditioned, symmetric positive definite diagonal value used throughout (in [1, 3])
static double diag_value(const size_t ii)
{
  return 1. + double(ii % 3);
}


GTEST_TEST(operators_matrix, apply_equals_matrix_times_vector)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  const auto pattern = make_element_sparsity_pattern(space);
  M matrix(n, n, pattern);
  for (size_t ii = 0; ii < n; ++ii)
    matrix.set_entry(ii, ii, diag_value(ii));

  auto op = make_matrix_operator(space, matrix);

  V source(n, 0.);
  for (size_t ii = 0; ii < n; ++ii)
    source.set_entry(ii, 2. + double(ii));

  // reference: compute matrix * source directly via the container's mv
  V expected(n, 0.);
  matrix.mv(source, expected);

  const auto range = op.apply(source);
  ASSERT_EQ(n, range.size());
  for (size_t ii = 0; ii < n; ++ii) {
    EXPECT_DOUBLE_EQ(diag_value(ii) * source.get_entry(ii), range.get_entry(ii));
    EXPECT_DOUBLE_EQ(expected.get_entry(ii), range.get_entry(ii));
  }
}


GTEST_TEST(operators_matrix, apply_inverse_round_trips)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  const auto pattern = make_element_sparsity_pattern(space);
  M matrix(n, n, pattern);
  for (size_t ii = 0; ii < n; ++ii)
    matrix.set_entry(ii, ii, diag_value(ii));

  auto op = make_matrix_operator(space, matrix);

  V source(n, 0.);
  for (size_t ii = 0; ii < n; ++ii)
    source.set_entry(ii, 2. + double(ii));

  const auto range = op.apply(source);
  const auto recovered = op.apply_inverse(range);
  ASSERT_EQ(n, recovered.size());
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_NEAR(source.get_entry(ii), recovered.get_entry(ii), 1e-10);
}


GTEST_TEST(operators_matrix, jacobian_returns_the_matrix)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  const auto pattern = make_element_sparsity_pattern(space);
  M matrix(n, n, pattern);
  for (size_t ii = 0; ii < n; ++ii)
    matrix.set_entry(ii, ii, diag_value(ii));

  auto op = make_matrix_operator(space, matrix);

  V source(n, 0.);
  const auto jac = op.jacobian(source);
  const auto& jac_matrix = jac.matrix();
  ASSERT_EQ(n, jac_matrix.rows());
  ASSERT_EQ(n, jac_matrix.cols());
  // the jacobian of a linear matrix operator is the matrix itself (scaling == 1)
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(matrix.get_entry(ii, ii), jac_matrix.get_entry(ii, ii));
}
