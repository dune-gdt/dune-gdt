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

#include <dune/xt/common/configuration.hh>
#include <dune/xt/common/string.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/la/container/istl.hh>

#include <dune/gdt/operators/lincomb.hh>
#include <dune/gdt/operators/matrix.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/tools/sparsity-pattern.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using M = XT::LA::IstlRowMajorSparseMatrix<double>;
using V = XT::LA::IstlDenseVector<double>;


GTEST_TEST(operators_lincomb, apply_and_jacobian_are_the_linear_combination)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  const auto pattern = make_element_sparsity_pattern(space);

  // two diagonal matrix operators A and B (must outlive the lincomb, which references them)
  M matrix_a(n, n, pattern);
  M matrix_b(n, n, pattern);
  for (size_t ii = 0; ii < n; ++ii) {
    matrix_a.set_entry(ii, ii, 1. + double(ii % 3)); // a_ii in [1, 3]
    matrix_b.set_entry(ii, ii, 2. + double(ii % 2)); // b_ii in [2, 3]
  }
  auto op_a = make_matrix_operator(space, matrix_a);
  auto op_b = make_matrix_operator(space, matrix_b);

  const double a = 2.;
  const double b = 3.;

  auto lincomb = make_lincomb_operator(space);
  lincomb.add(op_a, a);
  lincomb.add(op_b, b);

  // the accessors report the two summands with their coefficients
  ASSERT_EQ(2u, lincomb.num_ops());
  EXPECT_DOUBLE_EQ(a, lincomb.coeff(0));
  EXPECT_DOUBLE_EQ(b, lincomb.coeff(1));

  V source(n, 0.);
  for (size_t ii = 0; ii < n; ++ii)
    source.set_entry(ii, 1. + double(ii));

  // apply(v) == a * A.apply(v) + b * B.apply(v)
  const auto range = lincomb.apply(source);
  const auto range_a = op_a.apply(source);
  const auto range_b = op_b.apply(source);
  ASSERT_EQ(n, range.size());
  for (size_t ii = 0; ii < n; ++ii) {
    const double expected = a * range_a.get_entry(ii) + b * range_b.get_entry(ii);
    EXPECT_DOUBLE_EQ(expected, range.get_entry(ii));
  }

  // the jacobian is a * A + b * B (diagonal since both summands are diagonal)
  const auto jac = lincomb.jacobian(source);
  const auto& jac_matrix = jac.matrix();
  ASSERT_EQ(n, jac_matrix.rows());
  for (size_t ii = 0; ii < n; ++ii) {
    const double expected = a * matrix_a.get_entry(ii, ii) + b * matrix_b.get_entry(ii, ii);
    EXPECT_DOUBLE_EQ(expected, jac_matrix.get_entry(ii, ii));
  }
}


GTEST_TEST(operators_lincomb, operator_algebra_scaling_and_sum)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  const auto pattern = make_element_sparsity_pattern(space);
  M matrix_a(n, n, pattern);
  M matrix_b(n, n, pattern);
  for (size_t ii = 0; ii < n; ++ii) {
    matrix_a.set_entry(ii, ii, 1. + double(ii % 3));
    matrix_b.set_entry(ii, ii, 2. + double(ii % 2));
  }
  auto op_a = make_matrix_operator(space, matrix_a);
  auto op_b = make_matrix_operator(space, matrix_b);

  V source(n, 0.);
  for (size_t ii = 0; ii < n; ++ii)
    source.set_entry(ii, 1. + double(ii));

  // build (A + B) via operator+ and scale it in place by *= 2
  auto sum = op_a + op_b;
  sum *= 2.;
  ASSERT_EQ(2u, sum.num_ops());

  const auto range = sum.apply(source);
  const auto range_a = op_a.apply(source);
  const auto range_b = op_b.apply(source);
  for (size_t ii = 0; ii < n; ++ii) {
    const double expected = 2. * (range_a.get_entry(ii) + range_b.get_entry(ii));
    EXPECT_DOUBLE_EQ(expected, range.get_entry(ii));
  }
}


GTEST_TEST(operators_lincomb, jacobian_with_structured_per_summand_opts)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  const auto pattern = make_element_sparsity_pattern(space);
  M matrix_a(n, n, pattern);
  M matrix_b(n, n, pattern);
  for (size_t ii = 0; ii < n; ++ii) {
    matrix_a.set_entry(ii, ii, 1. + double(ii % 3));
    matrix_b.set_entry(ii, ii, 2. + double(ii % 2));
  }
  auto op_a = make_matrix_operator(space, matrix_a);
  auto op_b = make_matrix_operator(space, matrix_b);

  const double a = 2.;
  const double b = 3.;
  auto lincomb = make_lincomb_operator(space);
  lincomb.add(op_a, a);
  lincomb.add(op_b, b);

  V source(n, 0.);
  for (size_t ii = 0; ii < n; ++ii)
    source.set_entry(ii, 1. + double(ii));

  // The plain jacobian(source) forwards the operator's default jacobian_options(): those carry no "op.<i>.opts"
  // subtree, so inside LincombOperator::jacobian the two has_sub(...) checks are false and op_opts stays empty (the
  // if (op_opts.empty()) branch is taken).
  const auto jac_default = lincomb.jacobian(source);

  // Passing structured per-summand opts flips all three: each "op.<i>.opts"/"back_op.<i>.opts" subtree makes the
  // has_sub(...) checks true, and op_opts is then non-empty. The per-summand type is "matrix" (the summands are
  // matrix operators), so both routes assemble the same linear combination.
  XT::Common::Configuration opts;
  opts["type"] = "lincomb";
  for (size_t ii = 0; ii < lincomb.num_ops(); ++ii) {
    const auto op_prefix = "op." + XT::Common::to_string(ii);
    const auto back_op_prefix = "back_op." + XT::Common::to_string(ii);
    opts[op_prefix + ".type"] = "matrix";
    opts[op_prefix + ".opts.type"] = "matrix";
    opts[back_op_prefix + ".type"] = "matrix";
    opts[back_op_prefix + ".opts.type"] = "matrix";
  }
  const auto jac_structured = lincomb.jacobian(source, opts);

  const auto& m_default = jac_default.matrix();
  const auto& m_structured = jac_structured.matrix();
  ASSERT_EQ(n, m_default.rows());
  ASSERT_EQ(m_default.rows(), m_structured.rows());
  for (size_t ii = 0; ii < n; ++ii) {
    const double expected = a * matrix_a.get_entry(ii, ii) + b * matrix_b.get_entry(ii, ii);
    EXPECT_DOUBLE_EQ(expected, m_default.get_entry(ii, ii));
    EXPECT_DOUBLE_EQ(expected, m_structured.get_entry(ii, ii));
  }
}
