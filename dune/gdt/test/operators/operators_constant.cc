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

#include <dune/gdt/operators/constant.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using V = XT::LA::IstlDenseVector<double>;


GTEST_TEST(operators_constant, apply_returns_fixed_value_regardless_of_source)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  // the fixed constant range value (must outlive the operator, which holds it by reference)
  V value(n, 0.);
  for (size_t ii = 0; ii < n; ++ii)
    value.set_entry(ii, 1. + double(ii));

  auto op = make_constant_operator(space, value);

  // first source
  V source_a(n, 0.);
  for (size_t ii = 0; ii < n; ++ii)
    source_a.set_entry(ii, 42. - double(ii));
  const auto range_a = op.apply(source_a);
  ASSERT_EQ(n, range_a.size());
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(value.get_entry(ii), range_a.get_entry(ii));

  // a completely different source has to yield the very same range
  V source_b(n, -7.);
  const auto range_b = op.apply(source_b);
  ASSERT_EQ(n, range_b.size());
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(value.get_entry(ii), range_b.get_entry(ii));
}
