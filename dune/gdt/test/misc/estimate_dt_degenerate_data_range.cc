// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

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


/// The dt estimate has to cope with state components of constant value (e.g. the initial momentum of an Euler problem
/// at rest is constant 0): the bounding box of the state range is degenerate then and used to be "fixed" by an additive
/// perturbation of 1e-6 * minimum, which stays degenerate for a constant-zero and becomes inverted (maximum < minimum)
/// for a constant-negative component, so that the estimate crashed instead of returning a dt.
struct EstimateDtForDegenerateDataRangeTest : public ::testing::Test
{
  using StateFunctionType = XT::Functions::GenericFunction<1, 1, 1>;
  using StateWrapperType = XT::Functions::FunctionAsGridFunctionWrapper<E, 1, 1, double>;

  double estimate_dt_for_constant_state(const double value)
  {
    auto grid_provider = XT::Grid::make_cube_grid<G>(0., 1., 4u);
    auto grid_view = grid_provider.leaf_view();
    const StateFunctionType constant_state(
        0, [=](const auto& /*x*/, const auto& /*param*/) { return value; }, "constant_state");
    const StateWrapperType state(constant_state);
    // linear flux f(u) = u, i.e. |f'| = 1, so the expected dt is 1 / (perimeter/volume * max|f'|) = h/2
    const XT::Functions::GenericFunction<1, 1, 1> flux(
        1,
        [](const auto& u, const auto& /*param*/) { return u; },
        "linear",
        {},
        [](const auto& u, const auto& /*param*/) { return decltype(u)(1.); });
    return estimate_dt_for_hyperbolic_system(grid_view, state, flux);
  }
}; // struct EstimateDtForDegenerateDataRangeTest


TEST_F(EstimateDtForDegenerateDataRangeTest, constant_positive_state)
{
  EXPECT_NEAR(estimate_dt_for_constant_state(1.), 0.125, 1e-10);
}

TEST_F(EstimateDtForDegenerateDataRangeTest, constant_zero_state)
{
  // used to construct a YaspGrid with an empty bounding box [0, 0]
  EXPECT_NEAR(estimate_dt_for_constant_state(0.), 0.125, 1e-10);
}

TEST_F(EstimateDtForDegenerateDataRangeTest, constant_negative_state)
{
  // used to construct a YaspGrid with an inverted bounding box [-1, -1.000001]
  EXPECT_NEAR(estimate_dt_for_constant_state(-1.), 0.125, 1e-10);
}
