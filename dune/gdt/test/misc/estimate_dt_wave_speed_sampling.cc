// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <limits>

#include <dune/xt/functions/base/function-as-grid-function.hh>
#include <dune/xt/functions/generic/function.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/tools/hyperbolic.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_1D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using E = XT::Grid::extract_entity_t<GV>;

using StateWrapperType = XT::Functions::FunctionAsGridFunctionWrapper<E, 1, 1, double>;


/// For a convex flux the maximal wave speed over the state range [u_min, u_max] is attained at the boundary of the
/// range, but the estimate used to sample the flux derivative only at (interior) Gauss quadrature points, so max|f'|
/// was systematically underestimated and the resulting dt was no upper bound for the CFL-stable time step length.
GTEST_TEST(EstimateDtForHyperbolicSystem, samples_wave_speed_at_data_range_boundary)
{
  auto grid_provider = XT::Grid::make_cube_grid<G>(0., 1., 4u);
  auto grid_view = grid_provider.leaf_view();
  // piecewise constant state attaining the values 0 and 1 at element quadrature points, i.e. the estimated state
  // range is exactly [0, 1] (a continuous state like u(x) = x would only be sampled at interior quadrature points
  // of the elements, shrinking the estimated range - a separate approximation that is not under test here)
  const XT::Functions::GenericFunction<1, 1, 1> step_state(
      0, [](const auto& x, const auto& /*param*/) { return x[0] < 0.5 ? 0. : 1.; }, "step_state");
  const StateWrapperType state(step_state);
  // Burgers flux: f(u) = u^2/2, f'(u) = u, so max|f'| = 1 on [0, 1] and the expected dt is
  // 1 / (perimeter/volume * max|f'|) = 1 / (2/h * 1) = h/2 = 0.125
  // (the two-point Gauss rule on [0, 1] samples u = 0.211 and u = 0.789 only, leading to dt = 0.158 before the fix)
  const XT::Functions::GenericFunction<1, 1, 1> burgers_flux(
      2,
      [](const auto& u, const auto& /*param*/) { return 0.5 * u * u; },
      "burgers",
      {},
      [](const auto& u, const auto& /*param*/) { return u; });
  const auto dt = estimate_dt_for_hyperbolic_system(grid_view, state, burgers_flux);
  EXPECT_NEAR(dt, 0.125, 1e-10);
}

GTEST_TEST(EstimateDtForHyperbolicSystem, returns_max_for_constant_flux)
{
  auto grid_provider = XT::Grid::make_cube_grid<G>(0., 1., 4u);
  auto grid_view = grid_provider.leaf_view();
  const XT::Functions::GenericFunction<1, 1, 1> some_state(
      1, [](const auto& x, const auto& /*param*/) { return x; }, "linear_state");
  const StateWrapperType state(some_state);
  // a constant flux does not transport anything, so there is no CFL restriction at all
  const XT::Functions::GenericFunction<1, 1, 1> constant_flux(
      0,
      [](const auto& /*u*/, const auto& /*param*/) { return 1.; },
      "constant",
      {},
      [](const auto& u, const auto& /*param*/) { return decltype(u)(0.); });
  const auto dt = estimate_dt_for_hyperbolic_system(grid_view, state, constant_flux);
  EXPECT_EQ(dt, std::numeric_limits<double>::max());
}
