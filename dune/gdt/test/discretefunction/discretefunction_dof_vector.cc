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
#include <dune/gdt/exceptions.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using E = XT::Grid::extract_entity_t<GV>;
using V = XT::LA::IstlDenseVector<double>;


// The DofVector wrapper exposes the underlying global vector, whose set/get/add access has to round-trip exactly.
GTEST_TEST(discretefunction_dof_vector, global_vector_round_trip)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);

  auto u = make_discrete_function<V>(space);
  auto& vec = u.dofs().vector();
  EXPECT_EQ(space.mapper().size(), vec.size());

  vec.set_entry(0, 3.);
  EXPECT_DOUBLE_EQ(3., vec.get_entry(0));
  vec.add_to_entry(0, 4.);
  EXPECT_DOUBLE_EQ(7., vec.get_entry(0));
}


// A mutable localized view maps local to global DoFs; set/get/add have to round-trip and, crucially, have to write
// through to the global vector (verified via a second, independent localized view and via the global vector sum).
GTEST_TEST(discretefunction_dof_vector, local_view_set_get_add_round_trip)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  auto u = make_discrete_function<V>(space); // default constructed: all DoFs zero
  const auto element = *grid_view.template begin<0>();

  auto local = u.dofs().localize();
  local.bind(element);
  const auto sz = local.size();
  ASSERT_GT(sz, 0u);

  for (size_t ii = 0; ii < sz; ++ii)
    local.set_entry(ii, double(ii) + 1.);
  for (size_t ii = 0; ii < sz; ++ii)
    EXPECT_DOUBLE_EQ(double(ii) + 1., local.get_entry(ii));

  local.add_to_entry(0, 10.);
  EXPECT_DOUBLE_EQ(11., local.get_entry(0));

  // a fresh localized view onto the same element has to observe the same (global) values
  auto other = u.dofs().localize();
  other.bind(element);
  double local_sum = 0.;
  for (size_t ii = 0; ii < sz; ++ii) {
    EXPECT_DOUBLE_EQ(local.get_entry(ii), other.get_entry(ii));
    local_sum += local.get_entry(ii);
  }

  // only the DoFs of this single element were touched, all remaining global DoFs are still zero
  double global_sum = 0.;
  for (size_t ii = 0; ii < n; ++ii)
    global_sum += u.dofs().vector().get_entry(ii);
  EXPECT_DOUBLE_EQ(local_sum, global_sum);
}


// A const localized view provides read-only access to the global DoFs on the bound element.
GTEST_TEST(discretefunction_dof_vector, const_local_view_reads_global)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  auto u = make_discrete_function<V>(space);
  for (size_t ii = 0; ii < n; ++ii)
    u.dofs().vector().set_entry(ii, 1.);

  const auto& const_u = u;
  auto const_local = const_u.dofs().localize();
  const auto element = *grid_view.template begin<0>();
  const_local.bind(element);
  ASSERT_GT(const_local.size(), 0u);
  for (size_t ii = 0; ii < const_local.size(); ++ii)
    EXPECT_DOUBLE_EQ(1., const_local.get_entry(ii));
}


// Querying the size of an unbound localized view is an error.
GTEST_TEST(discretefunction_dof_vector, size_throws_before_bind)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);

  auto u = make_discrete_function<V>(space);
  auto local = u.dofs().localize();
  EXPECT_THROW(local.size(), Exceptions::not_bound_to_an_element_yet);
}
