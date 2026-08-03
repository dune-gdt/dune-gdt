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
#include <dune/xt/functions/grid-function.hh>
#include <dune/xt/la/container/istl.hh>

#include <dune/gdt/functionals/l2.hh>
#include <dune/gdt/functionals/localizable-functional.hh>
#include <dune/gdt/functionals/vector-based.hh>
#include <dune/gdt/local/functionals/integrals.hh>
#include <dune/gdt/local/integrands/identity.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using E = XT::Grid::extract_entity_t<GV>;
using V = XT::LA::IstlDenseVector<double>;


// For f == 1 on a continuous Lagrange P1 space each entry of the assembled vector equals \int f phi_i = \int phi_i >=
// 0, and since the hat functions form a partition of unity we have \sum_i \int phi_i = \int \sum_i phi_i = \int 1 =
// |Omega|. The default unit cube has volume 1.
GTEST_TEST(functionals_l2, assembles_integral_against_basis)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  const XT::Functions::GridFunction<E> one(1.);
  auto functional = make_l2_volume_vector_functional<V>(space, one);
  functional.assemble();

  const auto& vec = functional.vector();
  ASSERT_EQ(n, vec.size());

  double sum = 0.;
  for (size_t ii = 0; ii < n; ++ii) {
    EXPECT_GE(vec.get_entry(ii), 0.);
    sum += vec.get_entry(ii);
  }
  EXPECT_NEAR(1., sum, 1e-12);
}


// Applying the L2 functional (assembled against f == 1) to the constant discrete function u == 1 (all DoFs equal to
// one) yields vec . u = \sum_i vec_i = \int 1 * 1 = |Omega| = 1.
GTEST_TEST(functionals_l2, apply_equals_dot_product)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  const XT::Functions::GridFunction<E> one(1.);
  auto functional = make_l2_volume_vector_functional<V>(space, one);
  functional.assemble();

  const V ones(n, 1.);
  EXPECT_NEAR(1., functional.apply(ones), 1e-12);
}


// A ConstVectorBasedFunctional wraps a given vector; it is linear and its application is the dot product with the
// source vector.
GTEST_TEST(functionals_l2, const_vector_based_functional_is_dot_product)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  const auto n = space.mapper().size();

  V weights(n, 0.);
  for (size_t ii = 0; ii < n; ++ii)
    weights.set_entry(ii, double(ii) + 1.);
  auto functional = make_vector_functional(space, weights);

  EXPECT_TRUE(functional.linear());
  EXPECT_EQ(n, functional.vector().size());

  V source(n, 2.);
  double expected = 0.;
  for (size_t ii = 0; ii < n; ++ii)
    expected += 2. * (double(ii) + 1.);
  EXPECT_NEAR(expected, functional.apply(source), 1e-12);
}


// The localizable functional accumulates a scalar by walking the grid view. Using the identity integrand against the
// constant source f == 1 computes \int_Omega f = |Omega| = 1.
GTEST_TEST(functionals_l2, localizable_functional_accumulates_integral)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();

  const XT::Functions::GridFunction<E> one(1.);
  auto functional = make_localizable_functional(grid_view, one);
  functional.append(LocalElementIntegralFunctional<E>(LocalElementIdentityIntegrand<E>()));
  functional.assemble();

  EXPECT_NEAR(1., functional.result(), 1e-12);
}
