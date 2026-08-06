// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <cmath>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/la/container/istl.hh>

#include <dune/gdt/discretefunction/default.hh>
#include <dune/gdt/interpolations/default.hh>
#include <dune/gdt/local/operators/advection-dg.hh>
#include <dune/gdt/spaces/l2/discontinuous-lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using V = XT::LA::IstlDenseVector<double>;


/// The artificial viscosity shock capturing has to vanish for a continuous solution without jumps. The jump
/// indicator normalization used to be applied within the intersection loop, which on elements touching the domain
/// boundary in 2d divided by a (partially) zero accumulated boundary measure (0/0 = NaN), switching the viscosity
/// fully on even for smooth data (and making the indicator dependent on the intersection iteration order).
GTEST_TEST(AdvectionDgArtificialViscosityShockCapturing, vanishes_for_continuous_solutions)
{
  auto grid_provider = XT::Grid::make_cube_grid<G>(0., 1., 4u);
  auto grid_view = grid_provider.leaf_view();
  const auto space = make_discontinuous_lagrange_space(grid_view, /*order=*/1);

  // u(x) = x_0 is contained in the DG space and continuous, so there are no jumps and no viscosity may be added
  const auto u = default_interpolation<V>(1, [](const auto& xx, const auto& /*mu*/) { return xx[0]; }, space);

  LocalAdvectionDgArtificialViscosityShockCapturingOperator<V, GV> artificial_viscosity_operator(u, grid_view);
  DiscreteFunction<V, GV> range(space);
  auto local_range = range.local_discrete_function();
  for (auto&& element : elements(grid_view)) {
    artificial_viscosity_operator.bind(element);
    local_range->bind(element);
    artificial_viscosity_operator.apply(*local_range);
  }

  for (size_t ii = 0; ii < space.mapper().size(); ++ii) {
    EXPECT_TRUE(std::isfinite(range.dofs().vector()[ii])) << "ii = " << ii;
    EXPECT_NEAR(range.dofs().vector()[ii], 0., 1e-12) << "ii = " << ii;
  }
}
