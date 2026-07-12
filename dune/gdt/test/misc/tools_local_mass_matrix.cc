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

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/walker.hh>

#include <dune/gdt/spaces/l2/finite-volume.hh>
#include <dune/gdt/tools/local-mass-matrix.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;


GTEST_TEST(local_mass_matrix, finite_volume_local_mass_matrix_equals_element_volume)
{
  auto grid = XT::Grid::make_cube_grid<G>(/*lower_left=*/0., /*upper_right=*/1., /*num_elements=*/2);
  auto grid_view = grid.leaf_view();
  // scalar finite volume space: the single local basis function is the constant 1 on each element, so the local mass
  // matrix is the 1 x 1 matrix ( \int_E 1 * 1 ) = ( |E| ) and its inverse is ( 1 / |E| ).
  auto fv_space = make_finite_volume_space<1>(grid_view);

  LocalMassMatrixProvider<GV> provider(grid_view, fv_space);
  auto walker = XT::Grid::make_walker(grid_view);
  walker.append(provider);
  walker.walk();

  for (auto&& element : elements(grid_view)) {
    const double volume = element.geometry().volume();

    const auto& mass_matrix = provider.local_mass_matrix(element);
    ASSERT_EQ(1u, mass_matrix.rows());
    ASSERT_EQ(1u, mass_matrix.cols());
    EXPECT_DOUBLE_EQ(volume, mass_matrix.get_entry(0, 0));

    const auto& mass_matrix_inverse = provider.local_mass_matrix_inverse(element);
    ASSERT_EQ(1u, mass_matrix_inverse.rows());
    ASSERT_EQ(1u, mass_matrix_inverse.cols());
    EXPECT_DOUBLE_EQ(1. / volume, mass_matrix_inverse.get_entry(0, 0));
  }
}
