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
#include <cmath>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/string.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/la/container/istl.hh>

#include <dune/gdt/discretefunction/default.hh>
#include <dune/gdt/discretefunction/reinterpret.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
static constexpr size_t d = G::dimension;
using GV = typename G::LeafGridView;
using E = XT::Grid::extract_entity_t<GV>;
using V = XT::LA::IstlDenseVector<double>;


namespace {


XT::Grid::GridProvider<G> make_grid(const std::string& num_elements, const std::string& upper_right = "[1 1]")
{
  return XT::Grid::make_cube_grid<G>(XT::Common::from_string<FieldVector<double, d>>("[0 0]"),
                                     XT::Common::from_string<FieldVector<double, d>>(upper_right),
                                     XT::Common::from_string<std::array<unsigned int, d>>(num_elements));
}


// A function which is easy to reproduce by hand, evaluated at the center of the element x lies in.
double reference_value(const FieldVector<double, d>& x)
{
  return 1. + x[0] + 10. * x[1];
}


// Fills the DoFs of a finite volume function such that it equals reference_value(element_center) on each element.
template <class SpaceType>
void fill_dofs(const SpaceType& space, DiscreteFunction<V, GV>& u)
{
  auto local_dofs = u.dofs().localize();
  for (auto&& element : elements(space.grid_view())) {
    local_dofs.bind(element);
    local_dofs.set_entry(0, reference_value(element.geometry().center()));
  }
}


} // namespace


// Reinterpreting onto the very grid view the source lives on has to reproduce the source exactly.
GTEST_TEST(discretefunction_reinterpret, round_trips_on_the_source_grid_view)
{
  auto grid = make_grid("[2 2]");
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  auto u = make_discrete_function<V>(space);
  fill_dofs(space, u);

  const auto reinterpreted = reinterpret(u, grid_view);
  auto local_u = u.local_function();
  auto local_reinterpreted = reinterpreted.local_function();
  size_t num_elements = 0;
  for (auto&& element : elements(grid_view)) {
    local_u->bind(element);
    local_reinterpreted->bind(element);
    const auto x_in_reference_element = element.geometry().local(element.geometry().center());
    EXPECT_DOUBLE_EQ(local_u->evaluate(x_in_reference_element)[0],
                     local_reinterpreted->evaluate(x_in_reference_element)[0]);
    ++num_elements;
  }
  EXPECT_EQ(4u, num_elements);
}


// The same, but using the overload where the target element type is given explicitly instead of being deduced.
GTEST_TEST(discretefunction_reinterpret, round_trips_with_an_explicitly_given_target_element)
{
  auto grid = make_grid("[2 2]");
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  auto u = make_discrete_function<V>(space);
  fill_dofs(space, u);

  const auto reinterpreted = reinterpret<E>(u);
  auto local_u = u.local_function();
  auto local_reinterpreted = reinterpreted.local_function();
  for (auto&& element : elements(grid_view)) {
    local_u->bind(element);
    local_reinterpreted->bind(element);
    const auto x_in_reference_element = element.geometry().local(element.geometry().center());
    EXPECT_DOUBLE_EQ(local_u->evaluate(x_in_reference_element)[0],
                     local_reinterpreted->evaluate(x_in_reference_element)[0]);
  }
}


// Reinterpreting a piecewise constant function onto a finer grid has to yield the value of the coarse element which
// contains the respective fine element.
GTEST_TEST(discretefunction_reinterpret, evaluates_the_source_on_a_finer_target_grid)
{
  auto coarse_grid = make_grid("[2 2]");
  auto coarse_grid_view = coarse_grid.leaf_view();
  const auto coarse_space = make_finite_volume_space(coarse_grid_view);
  auto u = make_discrete_function<V>(coarse_space);
  fill_dofs(coarse_space, u);

  auto fine_grid = make_grid("[4 4]");
  auto fine_grid_view = fine_grid.leaf_view();

  const auto reinterpreted = reinterpret(u, fine_grid_view);
  auto local_reinterpreted = reinterpreted.local_function();
  size_t num_elements = 0;
  for (auto&& fine_element : elements(fine_grid_view)) {
    local_reinterpreted->bind(fine_element);
    const auto center = fine_element.geometry().center();
    // the coarse grid has two elements per direction over [0, 1], so this is the center of the containing one
    FieldVector<double, d> coarse_center(0.);
    for (size_t dd = 0; dd < d; ++dd)
      coarse_center[dd] = (std::floor(center[dd] * 2.) + 0.5) / 2.;
    EXPECT_NEAR(
        reference_value(coarse_center), local_reinterpreted->evaluate(fine_element.geometry().local(center))[0], 1e-12);
    ++num_elements;
  }
  EXPECT_EQ(16u, num_elements);
}


// Where the target grid is not covered by the source grid, the reinterpreted function is documented to be zero.
GTEST_TEST(discretefunction_reinterpret, is_zero_outside_of_the_source_grid)
{
  auto source_grid = make_grid("[2 2]", "[0.5 0.5]"); // <- covers only the lower left quarter of the target grid
  auto source_grid_view = source_grid.leaf_view();
  const auto source_space = make_finite_volume_space(source_grid_view);
  auto u = make_discrete_function<V>(source_space);
  fill_dofs(source_space, u);

  // the target elements are of the same size as the source ones, so their centers coincide where they overlap
  auto target_grid = make_grid("[4 4]");
  auto target_grid_view = target_grid.leaf_view();

  const auto reinterpreted = reinterpret(u, target_grid_view);
  auto local_reinterpreted = reinterpreted.local_function();
  size_t num_zero_elements = 0;
  size_t num_nonzero_elements = 0;
  for (auto&& element : elements(target_grid_view)) {
    local_reinterpreted->bind(element);
    const auto center = element.geometry().center();
    const auto value = local_reinterpreted->evaluate(element.geometry().local(center))[0];
    if (center[0] < 0.5 && center[1] < 0.5) {
      EXPECT_NEAR(reference_value(center), value, 1e-12);
      ++num_nonzero_elements;
    } else {
      EXPECT_DOUBLE_EQ(0., value);
      ++num_zero_elements;
    }
  }
  EXPECT_EQ(4u, num_nonzero_elements);
  EXPECT_EQ(12u, num_zero_elements);
}
