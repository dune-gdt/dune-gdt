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

#include <cmath>

#include <dune/xt/common/lapacke.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>

#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/tools/grid-quality-estimates.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;


namespace {


// The eigenvalues below come out of lapack, so they carry a few ulp of noise which differs between lapack
// implementations; the closed forms are pinned relatively rather than to the last bit.
constexpr double tolerance = 1e-10;

struct Constants
{
  double inverse;
  double combined;
};

/// \brief Both eigen-solver-based estimates on a continuous Lagrange space over the cube box [0, extent]^d.
template <class GridType>
Constants estimates(const int order, const unsigned int num_elements, const double extent = 1.)
{
  auto grid = XT::Grid::make_cube_grid<GridType>(/*lower_left=*/0., /*upper_right=*/extent, num_elements);
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, order);
  return {estimate_inverse_inequality_constant(space), estimate_combined_inverse_trace_inequality_constant(space)};
}

/// \brief Both estimates are positive, finite and independent of the mesh width of a uniform partition.
///
/// Both constants are defined so that the mesh width drops out (C_I / h is the square root of the smallest nonzero
/// generalized eigenvalue, C_M (1 + C_I) / h the smallest nonzero one of the face/element pair), so refining a
/// structured box - which keeps every element similar to itself - has to leave them alone.
template <class GridType>
void check_mesh_width_independence(const int order)
{
  const auto reference = estimates<GridType>(order, 1);
  ASSERT_TRUE(std::isfinite(reference.inverse)) << "order = " << order;
  ASSERT_TRUE(std::isfinite(reference.combined)) << "order = " << order;
  EXPECT_GT(reference.inverse, 0.) << "order = " << order;
  EXPECT_GT(reference.combined, 0.) << "order = " << order;
  for (unsigned int num_elements : {2u, 3u}) {
    const auto refined = estimates<GridType>(order, num_elements);
    EXPECT_NEAR(reference.inverse, refined.inverse, tolerance * reference.inverse)
        << "order = " << order << ", num_elements = " << num_elements;
    EXPECT_NEAR(reference.combined, refined.combined, tolerance * reference.combined)
        << "order = " << order << ", num_elements = " << num_elements;
  }
} // ... check_mesh_width_independence(...)

/// \brief Both estimates are invariant under scaling the whole domain, over a wide range of domain sizes.
///
/// Both constants are dimensionless (a length times an eigenvalue that scales as the inverse of that length), so
/// [0, s]^d has to give the same answer as the unit box for every s. This is the property that a fixed absolute
/// cutoff on "which eigenvalues count as zero" silently breaks: the H1/L2 spectrum is O(h^-2), so on a big enough
/// box it drops below any fixed cutoff (no constant computable at all), and on a small enough box the O(eps * max)
/// noise of the zero modes rises above it - and a negative one of those, taken through sqrt, yields a NaN that
/// std::max drops on the floor, leaving the estimate at its seed value.
template <class GridType>
void check_scale_invariance(const int order)
{
  const auto reference = estimates<GridType>(order, 2);
  for (double extent : {1.e-5, 1.e-3, 1.e3, 1.e5, 1.e6}) {
    const auto scaled = estimates<GridType>(order, 2, extent);
    EXPECT_NEAR(reference.inverse, scaled.inverse, tolerance * reference.inverse)
        << "order = " << order << ", extent = " << extent;
    EXPECT_NEAR(reference.combined, scaled.combined, tolerance * reference.combined)
        << "order = " << order << ", extent = " << extent;
  }
} // ... check_scale_invariance(...)


} // namespace


GTEST_TEST(grid_quality_estimates, element_to_intersection_equivalence_constant_on_uniform_cube)
{
  // 4 x 4 uniform partition of the unit square, so each element is a square of side h = 1 / 4.
  auto grid = XT::Grid::make_cube_grid<G>(/*lower_left=*/0., /*upper_right=*/1., /*num_elements=*/4);
  auto grid_view = grid.leaf_view();

  // XT::Grid::diameter() returns the largest distance between any two corners:
  //   element diameter      = h * sqrt(2) (the diagonal of the square)
  //   intersection diameter = h           (the length of an edge)
  // so the (constant over the uniform grid) ratio intersection_diameter / element_diameter is 1 / sqrt(2).
  const auto result = estimate_element_to_intersection_equivalence_constant(grid_view);
  EXPECT_NEAR(1. / std::sqrt(2.), result, 1e-13);
}


GTEST_TEST(grid_quality_estimates, inverse_inequality_constants_are_positive)
{
  auto grid = XT::Grid::make_cube_grid<G>(/*lower_left=*/0., /*upper_right=*/1., /*num_elements=*/2);
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);

  // both estimates rely on a generalized eigen-solver and throw dependency_missing without lapacke
  if (XT::Common::Lapacke::available()) {
    EXPECT_GT(estimate_inverse_inequality_constant(space), 0.);
    EXPECT_GT(estimate_combined_inverse_trace_inequality_constant(space), 0.);
  }
}


// The closed forms below are the values the estimators produce on a uniform partition of the unit box; they are
// independent of the mesh width (see check_mesh_width_independence) and, being purely geometric, identical for every
// grid manager meshing the same box the same way (see inverse_inequality_constants_agree_on_3d_cube_grids).
GTEST_TEST(grid_quality_estimates, inverse_inequality_constants_on_2d_cube_grid)
{
  if (!XT::Common::Lapacke::available())
    GTEST_SKIP() << "the generalized eigen-solver dependency (lapacke) is unavailable in this build";
  const auto order_1 = estimates<YASP_2D_EQUIDISTANT_OFFSET>(1, 2);
  EXPECT_NEAR(2. * std::sqrt(6.), order_1.inverse, tolerance * 2. * std::sqrt(6.));
  EXPECT_NEAR(4. * std::sqrt(2.), order_1.combined, tolerance * 4. * std::sqrt(2.));
  // Q_1 is contained in Q_2, and the minimizing mode of the inverse inequality is the same linear one in both, so the
  // inverse constant does not change with the order; the trace constant does.
  const auto order_2 = estimates<YASP_2D_EQUIDISTANT_OFFSET>(2, 2);
  EXPECT_NEAR(2. * std::sqrt(6.), order_2.inverse, tolerance * 2. * std::sqrt(6.));
  EXPECT_NEAR(6. * std::sqrt(2.), order_2.combined, tolerance * 6. * std::sqrt(2.));

  check_mesh_width_independence<YASP_2D_EQUIDISTANT_OFFSET>(1);
  check_mesh_width_independence<YASP_2D_EQUIDISTANT_OFFSET>(2);
  check_scale_invariance<YASP_2D_EQUIDISTANT_OFFSET>(1);
  check_scale_invariance<YASP_2D_EQUIDISTANT_OFFSET>(2);
}


GTEST_TEST(grid_quality_estimates, inverse_inequality_constants_on_3d_cube_grid)
{
  if (!XT::Common::Lapacke::available())
    GTEST_SKIP() << "the generalized eigen-solver dependency (lapacke) is unavailable in this build";
  for (int order : {1, 2}) {
    const auto constants = estimates<YASP_3D_EQUIDISTANT_OFFSET>(order, 2);
    EXPECT_NEAR(6., constants.inverse, tolerance * 6.) << "order = " << order;
    EXPECT_NEAR(6. * std::sqrt(3.), constants.combined, tolerance * 6. * std::sqrt(3.)) << "order = " << order;
    check_mesh_width_independence<YASP_3D_EQUIDISTANT_OFFSET>(order);
    check_scale_invariance<YASP_3D_EQUIDISTANT_OFFSET>(order);
  }
}


// #378: the bindings' hypothesis property test segfaulted the whole pytest process in the combined estimate on a 3d
// ALUGrid cube. The cause turned out to be a dangling grid on the python side rather than anything in the estimator
// (see the py::keep_alive in python/gdt/dune/gdt/spaces/), but until now this was the only grid this estimator had
// never been run on in C++ - which is why the crash could look like an estimator bug in the first place.
GTEST_TEST(grid_quality_estimates, inverse_inequality_constants_on_3d_alu_cube_grid)
{
  if (!XT::Common::Lapacke::available())
    GTEST_SKIP() << "the generalized eigen-solver dependency (lapacke) is unavailable in this build";
  for (int order : {1, 2}) {
    const auto constants = estimates<ALU_3D_CUBE>(order, 2);
    EXPECT_NEAR(6., constants.inverse, tolerance * 6.) << "order = " << order;
    EXPECT_NEAR(6. * std::sqrt(3.), constants.combined, tolerance * 6. * std::sqrt(3.)) << "order = " << order;
    check_mesh_width_independence<ALU_3D_CUBE>(order);
    check_scale_invariance<ALU_3D_CUBE>(order);
  }
}


// Both constants only depend on the element geometries, so meshing the same box with the same hexahedra has to give
// the same answer no matter which grid manager holds them.
GTEST_TEST(grid_quality_estimates, inverse_inequality_constants_agree_on_3d_cube_grids)
{
  if (!XT::Common::Lapacke::available())
    GTEST_SKIP() << "the generalized eigen-solver dependency (lapacke) is unavailable in this build";
  for (int order : {1, 2}) {
    const auto yasp = estimates<YASP_3D_EQUIDISTANT_OFFSET>(order, 2);
    const auto alu = estimates<ALU_3D_CUBE>(order, 2);
    EXPECT_NEAR(yasp.inverse, alu.inverse, tolerance * yasp.inverse) << "order = " << order;
    EXPECT_NEAR(yasp.combined, alu.combined, tolerance * yasp.combined) << "order = " << order;
  }
}


// Simplices have no closed form as tidy as the cubes above, but the estimates still have to be positive, finite and
// invariant under refining the structured box they are built on.
GTEST_TEST(grid_quality_estimates, inverse_inequality_constants_on_simplicial_grids)
{
  if (!XT::Common::Lapacke::available())
    GTEST_SKIP() << "the generalized eigen-solver dependency (lapacke) is unavailable in this build";
  for (int order : {1, 2}) {
    check_mesh_width_independence<ALU_2D_SIMPLEX_CONFORMING>(order);
    check_mesh_width_independence<ALU_3D_SIMPLEX_CONFORMING>(order);
  }
}
