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

#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/spaces/hdiv/raviart-thomas.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>
#include <dune/gdt/spaces/skeleton/finite-volume.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
static constexpr size_t d = G::dimension;
using D = typename G::ctype;


GTEST_TEST(spaces_l2_and_skeleton_and_rt, finite_volume_space)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);

  EXPECT_EQ(SpaceType::finite_volume, space.type());
  EXPECT_GT(space.mapper().size(), 0u);
  for (auto&& element : Dune::elements(grid_view)) {
    const auto basis = space.basis().localize(element);
    EXPECT_EQ(1u, basis->size());
    const auto values = basis->evaluate_set(FieldVector<D, d>(0.));
    ASSERT_EQ(1u, values.size());
  }
}


GTEST_TEST(spaces_l2_and_skeleton_and_rt, finite_volume_skeleton_space)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_skeleton_space(grid_view);

  EXPECT_EQ(SpaceType::finite_volume_skeleton, space.type());
  EXPECT_GT(space.mapper().size(), 0u);
  for (auto&& element : Dune::elements(grid_view)) {
    const auto basis = space.basis().localize(element);
    EXPECT_EQ(1u, basis->size());
    const auto values = basis->evaluate_set(FieldVector<D, d>(0.));
    ASSERT_EQ(1u, values.size());
  }
}


GTEST_TEST(spaces_l2_and_skeleton_and_rt, raviart_thomas_space)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_raviart_thomas_space(grid_view, 0);

  EXPECT_EQ(SpaceType::raviart_thomas, space.type());
  EXPECT_GT(space.mapper().size(), 0u);
  for (auto&& element : Dune::elements(grid_view)) {
    const auto basis = space.basis().localize(element);
    EXPECT_GT(basis->size(), 0u);
    const auto& reference_element = Dune::ReferenceElements<D, d>::general(element.type());
    const auto values = basis->evaluate_set(reference_element.position(0, 0));
    ASSERT_EQ(basis->size(), values.size());
  }
}


// The continuous-Lagrange space uses the DefaultGlobalBasis (dune/gdt/spaces/basis/default.hh); exercise it here.
GTEST_TEST(spaces_l2_and_skeleton_and_rt, continuous_lagrange_space_default_basis)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);

  EXPECT_EQ(SpaceType::continuous_lagrange, space.type());
  EXPECT_GT(space.mapper().size(), 0u);
  for (auto&& element : Dune::elements(grid_view)) {
    const auto basis = space.basis().localize(element);
    EXPECT_GT(basis->size(), 0u);
    EXPECT_EQ(1, basis->order());
    const auto values = basis->evaluate_set(FieldVector<D, d>(0.));
    ASSERT_EQ(basis->size(), values.size());
  }
}
