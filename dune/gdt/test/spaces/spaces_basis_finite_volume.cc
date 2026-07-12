// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2018)
//   René Fritze     (2018)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/fvector.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>

#include <dune/gdt/spaces/l2/finite-volume.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
static constexpr size_t d = G::dimension;
using D = typename G::ctype;
using R = double;


GTEST_TEST(spaces_basis_finite_volume, basis_has_size_one_and_order_zero)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);

  for (auto&& element : Dune::elements(grid_view)) {
    const auto basis = space.basis().localize(element);
    EXPECT_EQ(1u, basis->size());
    EXPECT_EQ(0, basis->order());
  }
}


// Mirrors basis_is_finite_volume_basis from spaces/base.hh: the single constant basis function evaluates to 1
// everywhere.
GTEST_TEST(spaces_basis_finite_volume, basis_evaluates_to_one)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);

  const double tolerance = 1e-15;
  for (auto&& element : Dune::elements(grid_view)) {
    const auto values = space.basis().localize(element)->evaluate_set(FieldVector<D, d>(0.));
    ASSERT_EQ(1u, values.size());
    EXPECT_TRUE(XT::Common::FloatCmp::eq(values.at(0), FieldVector<R, 1>(1.), tolerance, tolerance));
  }
}


// The single basis function is constant, hence its jacobian vanishes everywhere.
GTEST_TEST(spaces_basis_finite_volume, basis_jacobians_vanish)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);

  const double tolerance = 1e-15;
  for (auto&& element : Dune::elements(grid_view)) {
    const auto grads = space.basis().localize(element)->jacobians_of_set(FieldVector<D, d>(0.));
    ASSERT_EQ(1u, grads.size());
    EXPECT_TRUE(XT::Common::FloatCmp::eq(grads.at(0), decltype(grads[0])(0), tolerance, tolerance));
  }
}
