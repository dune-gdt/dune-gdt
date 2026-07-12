// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <algorithm>
#include <cmath>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/string.hh>
#include <dune/xt/la/container/istl.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/functions/generic/function.hh>

#include <dune/gdt/discretefunction/default.hh>
#include <dune/gdt/local/numerical-fluxes/upwind.hh>
#include <dune/gdt/local/operators/advection-fv.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>

#include <dune/gdt/test/integrands/integrands.hh> // <- provides the Grids2D type list and the DXTC typenames

using namespace Dune;
using namespace Dune::GDT;


template <class G>
struct LocalAdvectionFvCouplingOperatorTest : public ::testing::Test
{
  static constexpr size_t d = G::dimension;
  using GV = typename G::LeafGridView;
  using E = XT::Grid::extract_entity_t<GV>;
  using I = XT::Grid::extract_intersection_t<GV>;
  using V = XT::LA::IstlDenseVector<double>;
  using GP = XT::Grid::GridProvider<G>;

  // linear transport flux f(u) = direction * u, as in dune/gdt/test/linear-transport/base.hh
  using FluxType = XT::Functions::GenericFunction<1, d, 1>;
  using NumericalFluxType = NumericalUpwindFlux<I, d, 1, double>;
  using OperatorType = LocalAdvectionFvCouplingOperator<I, V, GV>;

  static GP make_grid()
  {
    return GP(XT::Grid::make_cube_grid<G>(XT::Common::from_string<FieldVector<double, d>>("[0 0 0 0]"),
                                          XT::Common::from_string<FieldVector<double, d>>("[1 1 1 1]"),
                                          XT::Common::from_string<std::array<unsigned int, d>>("[2 2 2 2]")));
  }

  static FluxType make_flux()
  {
    XT::Common::FieldVector<double, d> direction(0.);
    direction[0] = 1.;
    return FluxType(
        1,
        [direction](const auto& u, const auto& /*param*/) { return direction * u; },
        "linear_transport",
        {},
        [direction](const auto& /*u*/, const auto& /*param*/) { return direction; });
  }

  // On a single interior intersection the FV coupling operator adds
  //     + g * |intersection| / |inside element|   to the inside DoF, and
  //     - g * |intersection| / |outside element|  to the outside DoF,
  // with the *same* numerical flux value g. Hence, for any source and any flux, the operator is locally
  // conservative:  inside_update * |inside| == -( outside_update * |outside| ). We assert this exact invariant and
  // additionally require that at least one interior intersection produces a non-trivial (non-zero) update, so the
  // conservation statement is not vacuously true.
  void is_locally_conservative()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto fv_space = make_finite_volume_space(grid_view);
    const double c = 2.;
    auto source = make_discrete_function<V>(fv_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, c);
    auto flux = make_flux();
    NumericalFluxType numerical_flux(flux);
    OperatorType op(source, numerical_flux, /*source_is_elementwise_constant=*/true);
    const auto element = *(grid_view.template begin<0>());
    double max_abs_update = 0.;
    for (auto&& intersection : intersections(grid_view, element)) {
      if (!intersection.neighbor())
        continue;
      auto range = make_discrete_function<V>(fv_space);
      auto local_range_inside = range.local_discrete_function();
      auto local_range_outside = range.local_discrete_function();
      local_range_inside->bind(intersection.inside());
      local_range_outside->bind(intersection.outside());
      op.bind(intersection);
      op.apply(*local_range_inside, *local_range_outside);
      const double inside_update = local_range_inside->dofs().get_entry(0);
      const double outside_update = local_range_outside->dofs().get_entry(0);
      const double vol_inside = intersection.inside().geometry().volume();
      const double vol_outside = intersection.outside().geometry().volume();
      EXPECT_NEAR(inside_update * vol_inside, -1. * outside_update * vol_outside, 1e-12);
      max_abs_update = std::max(max_abs_update, std::abs(inside_update));
    }
    EXPECT_GT(max_abs_update, 0.);
  }

  void space_and_vector_constructor_yields_the_same_result()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto fv_space = make_finite_volume_space(grid_view);
    const double c = 2.;
    auto source = make_discrete_function<V>(fv_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, c);
    auto flux = make_flux();
    NumericalFluxType numerical_flux(flux);
    OperatorType op_from_source(source, numerical_flux, /*source_is_elementwise_constant=*/true);
    OperatorType op_from_space_and_vector(
        fv_space, source.dofs().vector(), numerical_flux, /*source_is_elementwise_constant=*/true);
    const auto element = *(grid_view.template begin<0>());
    for (auto&& intersection : intersections(grid_view, element)) {
      if (!intersection.neighbor())
        continue;
      auto range_a = make_discrete_function<V>(fv_space);
      auto range_b = make_discrete_function<V>(fv_space);
      auto lr_a_in = range_a.local_discrete_function();
      auto lr_a_out = range_a.local_discrete_function();
      auto lr_b_in = range_b.local_discrete_function();
      auto lr_b_out = range_b.local_discrete_function();
      lr_a_in->bind(intersection.inside());
      lr_a_out->bind(intersection.outside());
      lr_b_in->bind(intersection.inside());
      lr_b_out->bind(intersection.outside());
      op_from_source.bind(intersection);
      op_from_source.apply(*lr_a_in, *lr_a_out);
      op_from_space_and_vector.bind(intersection);
      op_from_space_and_vector.apply(*lr_b_in, *lr_b_out);
      EXPECT_DOUBLE_EQ(lr_a_in->dofs().get_entry(0), lr_b_in->dofs().get_entry(0));
      EXPECT_DOUBLE_EQ(lr_a_out->dofs().get_entry(0), lr_b_out->dofs().get_entry(0));
    }
  }

  void copy_yields_the_same_result()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto fv_space = make_finite_volume_space(grid_view);
    const double c = 2.;
    auto source = make_discrete_function<V>(fv_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, c);
    auto flux = make_flux();
    NumericalFluxType numerical_flux(flux);
    OperatorType op(source, numerical_flux, /*source_is_elementwise_constant=*/true);
    auto clone = op.copy();
    const auto element = *(grid_view.template begin<0>());
    for (auto&& intersection : intersections(grid_view, element)) {
      if (!intersection.neighbor())
        continue;
      auto range_a = make_discrete_function<V>(fv_space);
      auto range_b = make_discrete_function<V>(fv_space);
      auto lr_a_in = range_a.local_discrete_function();
      auto lr_a_out = range_a.local_discrete_function();
      auto lr_b_in = range_b.local_discrete_function();
      auto lr_b_out = range_b.local_discrete_function();
      lr_a_in->bind(intersection.inside());
      lr_a_out->bind(intersection.outside());
      lr_b_in->bind(intersection.inside());
      lr_b_out->bind(intersection.outside());
      op.bind(intersection);
      op.apply(*lr_a_in, *lr_a_out);
      clone->bind(intersection);
      clone->apply(*lr_b_in, *lr_b_out);
      EXPECT_DOUBLE_EQ(lr_a_in->dofs().get_entry(0), lr_b_in->dofs().get_entry(0));
      EXPECT_DOUBLE_EQ(lr_a_out->dofs().get_entry(0), lr_b_out->dofs().get_entry(0));
    }
  }
}; // struct LocalAdvectionFvCouplingOperatorTest


template <class G>
using LocalAdvectionFvCouplingOperatorTests = LocalAdvectionFvCouplingOperatorTest<G>;
TYPED_TEST_SUITE(LocalAdvectionFvCouplingOperatorTests, Grids2D);
TYPED_TEST(LocalAdvectionFvCouplingOperatorTests, is_locally_conservative)
{
  this->is_locally_conservative();
}
TYPED_TEST(LocalAdvectionFvCouplingOperatorTests, space_and_vector_constructor_yields_the_same_result)
{
  this->space_and_vector_constructor_yields_the_same_result();
}
TYPED_TEST(LocalAdvectionFvCouplingOperatorTests, copy_yields_the_same_result)
{
  this->copy_yields_the_same_result();
}
