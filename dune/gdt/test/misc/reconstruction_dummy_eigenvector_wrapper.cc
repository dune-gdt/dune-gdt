// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/xt/functions/base/function-as-flux-function.hh>
#include <dune/xt/functions/generic/function.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/operators/reconstruction/internal.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_1D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using E = XT::Grid::extract_entity_t<GV>;

// The wrapper used to fail to compile when instantiated: its compute_eigenvectors(_impl) overloads were
// const-qualified (the base class methods are not), so the marked overrides did not override anything.
GTEST_TEST(DummyEigenVectorWrapper, can_be_instantiated_and_applied)
{
  using FluxType = XT::Functions::StateFunctionAsFluxFunctionWrapper<E, 1, 1, 1, double>;
  using VectorType = FieldVector<double, 1>;
  const XT::Functions::GenericFunction<1, 1, 1> burgers_flux(
      2,
      [](const auto& u, const auto& /*param*/) { return 0.5 * u * u; },
      "burgers",
      {},
      [](const auto& u, const auto& /*param*/) { return u; });
  const FluxType flux(burgers_flux);

  auto grid_provider = XT::Grid::make_cube_grid<G>(0., 1., 2u);
  const auto element = *grid_provider.leaf_view().template begin<0>();

  internal::DummyEigenVectorWrapper<FluxType, VectorType> wrapper(flux, /*flux_is_affine=*/false);
  wrapper.compute_eigenvectors(element, {0.5}, {1.}, {});
  VectorType ret(0.);
  wrapper.apply_eigenvectors(0, {2.}, ret);
  EXPECT_DOUBLE_EQ(ret[0], 2.);
  wrapper.apply_inverse_eigenvectors(0, {3.}, ret);
  EXPECT_DOUBLE_EQ(ret[0], 3.);
  EXPECT_THROW(wrapper.eigenvectors(0), Dune::NotImplemented);
}
