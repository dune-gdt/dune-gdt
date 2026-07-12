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

#include <dune/xt/grid/boundaryinfo.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/grid/walker.hh>
#include <dune/xt/la/container/istl.hh>

#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/tools/dirichlet-constraints.hh>
#include <dune/gdt/tools/sparsity-pattern.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using I = XT::Grid::extract_intersection_t<GV>;
using M = XT::LA::IstlRowMajorSparseMatrix<double>;
using V = XT::LA::IstlDenseVector<double>;


GTEST_TEST(dirichlet_constraints, gathers_boundary_dofs_on_all_dirichlet_boundary)
{
  // 2 x 2 elements on the unit square -> 3 x 3 = 9 P1 vertices (DoFs), of which the single center vertex is interior,
  // so 8 vertices lie on the boundary.
  auto grid = XT::Grid::make_cube_grid<G>(/*lower_left=*/0., /*upper_right=*/1., /*num_elements=*/2);
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  ASSERT_EQ(9u, space.mapper().size());

  XT::Grid::AllDirichletBoundaryInfo<I> boundary_info;
  auto dirichlet_constraints = make_dirichlet_constraints(space, boundary_info);

  auto walker = XT::Grid::make_walker(grid_view);
  walker.append(dirichlet_constraints);
  walker.walk();

  // all boundary vertices are constrained, the interior center vertex is not
  EXPECT_EQ(8u, dirichlet_constraints.dirichlet_DoFs().size());
  EXPECT_LT(dirichlet_constraints.dirichlet_DoFs().size(), space.mapper().size());
}


GTEST_TEST(dirichlet_constraints, apply_turns_constrained_rows_into_identity_and_clears_rhs)
{
  auto grid = XT::Grid::make_cube_grid<G>(/*lower_left=*/0., /*upper_right=*/1., /*num_elements=*/2);
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);

  XT::Grid::AllDirichletBoundaryInfo<I> boundary_info;
  auto dirichlet_constraints = make_dirichlet_constraints(space, boundary_info);

  auto walker = XT::Grid::make_walker(grid_view);
  walker.append(dirichlet_constraints);
  walker.walk();

  // build a matrix (filled with 1s on its sparsity pattern) and a right hand side (filled with 1s)
  const auto n = space.mapper().size();
  const auto pattern = make_element_sparsity_pattern(space);
  M matrix(n, n, pattern);
  for (size_t ii = 0; ii < n; ++ii)
    for (const size_t jj : pattern.inner(ii))
      matrix.set_entry(ii, jj, 1.);
  V vector(n, 1.);

  dirichlet_constraints.apply(matrix, vector);

  for (const auto& dof : dirichlet_constraints.dirichlet_DoFs()) {
    // constrained rows become identity rows ...
    EXPECT_DOUBLE_EQ(1., matrix.get_entry(dof, dof));
    for (const size_t jj : pattern.inner(dof))
      if (jj != dof)
        EXPECT_DOUBLE_EQ(0., matrix.get_entry(dof, jj));
    // ... and the corresponding right hand side entries are cleared
    EXPECT_DOUBLE_EQ(0., vector[dof]);
  }
}
