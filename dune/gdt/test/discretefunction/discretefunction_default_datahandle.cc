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

#include <array>
#include <vector>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/string.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/la/container/istl.hh>

#include <dune/gdt/discretefunction/default.hh>
#include <dune/gdt/discretefunction/default-datahandle.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
static constexpr size_t d = G::dimension;
using GV = typename G::LeafGridView;
using V = XT::LA::IstlDenseVector<double>;
using DF = DiscreteFunction<V, GV>;


namespace {


XT::Grid::GridProvider<G> make_grid()
{
  return XT::Grid::make_cube_grid<G>(XT::Common::from_string<FieldVector<double, d>>("[0 0]"),
                                     XT::Common::from_string<FieldVector<double, d>>("[1 1]"),
                                     XT::Common::from_string<std::array<unsigned int, d>>("[2 2]"));
}


// Minimal stand-in for the message buffer the grid hands to gather()/scatter() when communicating.
struct TestMessageBuffer
{
  void write(const double& value)
  {
    data.push_back(value);
  }

  void read(double& value)
  {
    value = data.at(read_index);
    ++read_index;
  }

  std::vector<double> data;
  size_t read_index{0};
}; // struct TestMessageBuffer


} // namespace


// Only the DoFs attached to elements are communicated, and the size is constant unless stated otherwise.
GTEST_TEST(discretefunction_default_datahandle, contains_only_codim_zero)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  auto u = make_discrete_function<V>(space);

  DiscreteFunctionDataHandle<DF> handle(u);
  EXPECT_TRUE(handle.contains(int(d), 0));
  EXPECT_FALSE(handle.contains(int(d), 1));
  EXPECT_FALSE(handle.contains(int(d), int(d)));
  EXPECT_TRUE(handle.fixedSize(int(d), 0));

  DiscreteFunctionDataHandle<DF> non_fixed_size_handle(u, /*fixed_size=*/false);
  EXPECT_FALSE(non_fixed_size_handle.fixedSize(int(d), 0));
}


// The announced size has to match the number of local DoFs for elements, and be zero for anything else.
GTEST_TEST(discretefunction_default_datahandle, announces_the_local_size_for_elements)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  auto u = make_discrete_function<V>(space);

  const DiscreteFunctionDataHandle<DF> handle(u);
  size_t num_elements = 0;
  for (auto&& element : elements(grid_view)) {
    EXPECT_EQ(space.mapper().local_size(element), handle.size(element));
    ++num_elements;
  }
  EXPECT_EQ(4u, num_elements);

  const auto vertex = *grid_view.begin<GV::dimension>();
  EXPECT_EQ(0u, handle.size(vertex));
}


// Gathering the DoFs of a function and scattering them into another one has to reproduce them exactly.
GTEST_TEST(discretefunction_default_datahandle, gather_and_scatter_round_trip)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const size_t n = space.mapper().size();

  auto u = make_discrete_function<V>(space);
  for (size_t ii = 0; ii < n; ++ii)
    u.dofs().vector().set_entry(ii, double(ii) + 1.);

  const DiscreteFunctionDataHandle<DF> handle(u);
  TestMessageBuffer buffer;
  for (auto&& element : elements(grid_view))
    handle.gather(buffer, element);
  ASSERT_EQ(n, buffer.data.size());

  auto v = make_discrete_function<V>(space); // <- default constructed: all DoFs zero
  DiscreteFunctionDataHandle<DF> other_handle(v);
  for (auto&& element : elements(grid_view))
    other_handle.scatter(buffer, element, space.mapper().local_size(element));

  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(u.dofs().vector().get_entry(ii), v.dofs().vector().get_entry(ii));
}


// Entities of codimension other than zero carry no data, so gather()/scatter() have to be no-ops for them.
GTEST_TEST(discretefunction_default_datahandle, gather_and_scatter_ignore_higher_codimensions)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  auto u = make_discrete_function<V>(space);
  u.dofs().vector().set_entry(0, 42.);

  DiscreteFunctionDataHandle<DF> handle(u);
  const auto vertex = *grid_view.begin<GV::dimension>();

  TestMessageBuffer buffer;
  handle.gather(buffer, vertex);
  EXPECT_TRUE(buffer.data.empty());

  handle.scatter(buffer, vertex, 0);
  EXPECT_DOUBLE_EQ(42., u.dofs().vector().get_entry(0));
}
