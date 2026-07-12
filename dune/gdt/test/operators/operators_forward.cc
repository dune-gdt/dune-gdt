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
#include <dune/xt/functions/grid-function.hh>

#include <dune/gdt/operators/forward-operator.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using E = XT::Grid::extract_entity_t<GV>;
using V = XT::LA::IstlDenseVector<double>;


GTEST_TEST(operators_forward, interface_accessors)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);

  auto op = make_forward_operator(space);

  // an operator without any appended local operators is linear ...
  EXPECT_TRUE(op.linear());
  // ... and its range space is the one it was built on
  EXPECT_EQ(space.mapper().size(), op.range_space().mapper().size());
}


GTEST_TEST(operators_forward, apply_without_local_operators_yields_zero)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  auto op = make_forward_operator(space);

  // a non-zero constant source: the assembler walk clears the range and, since no local operators
  // are appended, leaves it at zero
  XT::Functions::GridFunction<E> source(1.);
  const auto range = op.apply(source);
  ASSERT_EQ(n, range.dofs().vector().size());
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(0., range.dofs().vector().get_entry(ii));
}
