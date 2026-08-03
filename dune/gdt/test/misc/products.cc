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
#ifndef DUNE_XT_COMMON_TEST_MAIN_ENABLE_TIMED_LOGGING
#  define DUNE_XT_COMMON_TEST_MAIN_ENABLE_TIMED_LOGGING 1
#endif
#ifndef DUNE_XT_COMMON_TEST_MAIN_ENABLE_INFO_LOGGING
#  define DUNE_XT_COMMON_TEST_MAIN_ENABLE_INFO_LOGGING 1
#endif
#ifndef DUNE_XT_COMMON_TEST_MAIN_ENABLE_DEBUG_LOGGING
#  define DUNE_XT_COMMON_TEST_MAIN_ENABLE_DEBUG_LOGGING 1
#endif

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <cmath>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/functions/grid-function.hh>

#include <dune/gdt/norms.hh>
#include <dune/gdt/products.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using E = XT::Grid::extract_entity_t<GV>;


GTEST_TEST(products, l2_product_scalar)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();

  XT::Functions::GridFunction<E> f(1.);
  XT::Functions::GridFunction<E> g(2.);

  auto l2 = L2Product(grid_view);
  // the default cube grid covers the unit square, so \int 1 * 1 = 1
  EXPECT_DOUBLE_EQ(1., l2.apply2(f, f));
  // \int 1 * 2 = 2
  EXPECT_DOUBLE_EQ(2., l2.apply2(f, g));
  // bilinear forms are symmetric
  EXPECT_DOUBLE_EQ(l2.apply2(f, g), l2.apply2(g, f));
  // L2Product(f, f) has to coincide with l2_norm(f)^2
  EXPECT_DOUBLE_EQ(std::pow(l2_norm(grid_view, f), 2), l2.apply2(f, f));

  // the optional weight scales the product
  auto weighted = L2Product(grid_view, 2.);
  EXPECT_DOUBLE_EQ(2., weighted.apply2(f, f));
}


GTEST_TEST(products, l2_product_vector)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();

  XT::Functions::GridFunction<E, 2> f({1., 1.});

  auto l2 = L2Product<2>(grid_view);
  // \int (1*1 + 1*1) = 2
  EXPECT_DOUBLE_EQ(2., l2.apply2(f, f));
}


GTEST_TEST(products, h1_semi_product_scalar)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();

  XT::Functions::GridFunction<E> f(1.);

  auto h1_semi = H1SemiProduct(grid_view);
  // the gradient of a constant vanishes
  EXPECT_DOUBLE_EQ(0., h1_semi.apply2(f, f));
}


GTEST_TEST(products, h1_product_scalar)
{
  auto grid = XT::Grid::make_cube_grid<G>();
  auto grid_view = grid.leaf_view();

  XT::Functions::GridFunction<E> f(1.);

  // single weight overload: L2 part (= 1) + H1 semi part (= 0)
  auto h1 = H1Product(grid_view);
  EXPECT_DOUBLE_EQ(1., h1.apply2(f, f));

  // separate L2 and H1 semi weights
  auto h1_weighted = H1Product(grid_view, 2., 3.);
  EXPECT_DOUBLE_EQ(2., h1_weighted.apply2(f, f));
}
