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
#include <dune/gdt/norms.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using E = XT::Grid::extract_entity_t<GV>;
using V = XT::LA::IstlDenseVector<double>;


GTEST_TEST(discrete_function, default_constructed_is_zero)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  auto u = make_discrete_function<V>(space);
  EXPECT_EQ(n, u.dofs().vector().size());
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(0., u.dofs().vector().get_entry(ii));
  EXPECT_EQ("dune.gdt.discretefunction", u.name());
}


GTEST_TEST(discrete_function, partition_of_unity_is_constant_one)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  auto u = make_discrete_function<V>(space);
  // setting all Lagrange coefficients to one yields the constant function 1
  for (size_t ii = 0; ii < n; ++ii)
    u.dofs().vector().set_entry(ii, 1.);
  EXPECT_DOUBLE_EQ(1., l2_norm(grid_view, u));
}


GTEST_TEST(discrete_function, addition_of_dofs)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  auto u = make_discrete_function<V>(space);
  for (size_t ii = 0; ii < n; ++ii)
    u.dofs().vector().set_entry(ii, 1.);

  u += u;
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(2., u.dofs().vector().get_entry(ii));
}


GTEST_TEST(discrete_function, const_discrete_function_from_vector)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  const V vector(n, 3.);
  auto u = make_discrete_function(space, vector);
  EXPECT_EQ(n, u.dofs().vector().size());
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(3., u.dofs().vector().get_entry(ii));
}
