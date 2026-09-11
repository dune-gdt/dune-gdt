// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/xt/functions/generic/function.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/local/numerical-fluxes/engquist-osher.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_1D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using I = XT::Grid::extract_intersection_t<GV>;

/// Checks the Engquist-Osher flux g(u, v) = f(0) n + \int_0^u max(f'(s) n, 0) ds + \int_0^v min(f'(s) n, 0) ds
/// against analytically computed values, in particular for negative states (which used to be silently dropped).
struct EngquistOsherFluxTest : public ::testing::Test
{
  using FluxType = XT::Functions::GenericFunction<1, 1, 1>;
  using NumericalFluxType = NumericalEngquistOsherFlux<I, 1, 1, double>;
  using StateType = typename NumericalFluxType::StateType;
  using LocalCoordType = typename NumericalFluxType::LocalIntersectionCoords;

  void SetUp() override
  {
    grid_provider_ = std::make_unique<XT::Grid::GridProvider<G>>(XT::Grid::make_cube_grid<G>(0., 1., 2u));
    auto grid_view = grid_provider_->leaf_view();
    for (auto&& element : elements(grid_view))
      for (auto&& intersection : intersections(grid_view, element))
        if (intersection.neighbor() && !inner_intersection_) {
          inner_intersection_ = std::make_unique<I>(intersection);
          normal_ = intersection.centerUnitOuterNormal();
        }
    ASSERT_NE(inner_intersection_, nullptr);
    ASSERT_DOUBLE_EQ(normal_[0], 1.); // intersections of the first element are visited first
  }

  double apply(const FluxType& flux, const double u, const double v, const double n) const
  {
    NumericalFluxType numerical_flux(flux);
    numerical_flux.bind(*inner_intersection_);
    return numerical_flux.apply(LocalCoordType(), StateType(u), StateType(v), {n})[0];
  }

  std::unique_ptr<XT::Grid::GridProvider<G>> grid_provider_;
  std::unique_ptr<I> inner_intersection_;
  FieldVector<double, 1> normal_;
}; // struct EngquistOsherFluxTest


TEST_F(EngquistOsherFluxTest, linear_flux)
{
  // f(u) = u, f'(u) = 1: the flux has to reduce to the upwind value f(u) n for n = 1 (and f(v) n for n = -1),
  // for positive as well as negative states
  const FluxType linear_flux(
      1,
      [](const auto& u, const auto& /*param*/) { return u; },
      "linear",
      {},
      [](const auto& u, const auto& /*param*/) { return decltype(u)(1.); });
  EXPECT_NEAR(apply(linear_flux, 2., 3., 1.), 2., 1e-14);
  EXPECT_NEAR(apply(linear_flux, -2., -1., 1.), -2., 1e-14); // <- used to yield 0.
  EXPECT_NEAR(apply(linear_flux, -2., 3., 1.), -2., 1e-14); // <- used to yield 0.
  EXPECT_NEAR(apply(linear_flux, 3., 2., -1.), -2., 1e-14);
  EXPECT_NEAR(apply(linear_flux, -1., -2., -1.), 2., 1e-14); // <- used to yield 0.
}

TEST_F(EngquistOsherFluxTest, burgers_flux)
{
  // f(u) = u^2/2, f'(u) = u: for n = 1 the Engquist-Osher flux is g(u, v) = max(u, 0)^2/2 + min(v, 0)^2/2
  const FluxType burgers_flux(
      2,
      [](const auto& u, const auto& /*param*/) { return 0.5 * u * u; },
      "burgers",
      {},
      [](const auto& u, const auto& /*param*/) { return u; });
  EXPECT_NEAR(apply(burgers_flux, 1., 2., 1.), 0.5, 1e-14);
  EXPECT_NEAR(apply(burgers_flux, -1., 1., 1.), 0., 1e-14); // transonic rarefaction
  EXPECT_NEAR(apply(burgers_flux, -2., -1., 1.), 0.5, 1e-14); // <- used to yield 0.
  EXPECT_NEAR(apply(burgers_flux, 1., -2., 1.), 2.5, 1e-14); // transonic shock, used to yield 0.5
  EXPECT_NEAR(apply(burgers_flux, 2., -2., 1.), 4., 1e-14); // <- used to yield 2.
}
