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
#include <dune/gdt/local/operators/advection-dg.hh>
#include <dune/gdt/spaces/l2/discontinuous-lagrange.hh>

#include <dune/gdt/test/integrands/integrands.hh> // <- provides the Grids2D type list and the DXTC typenames

using namespace Dune;
using namespace Dune::GDT;


template <class G>
struct LocalAdvectionDgOperatorTest : public ::testing::Test
{
  static constexpr size_t d = G::dimension;
  using GV = typename G::LeafGridView;
  using E = XT::Grid::extract_entity_t<GV>;
  using I = XT::Grid::extract_intersection_t<GV>;
  using V = XT::LA::IstlDenseVector<double>;
  using GP = XT::Grid::GridProvider<G>;

  using FluxType = XT::Functions::GenericFunction<1, d, 1>; // f(u) = direction * u
  using NumericalFluxType = NumericalUpwindFlux<I, d, 1, double>;
  using VolumeOperatorType = LocalAdvectionDgVolumeOperator<V, GV>;
  using CouplingOperatorType = LocalAdvectionDgCouplingOperator<I, V, GV>;

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

  // The DG volume operator adds  local_dof_i += \int_E -f(u) . \nabla phi_i  (no local mass matrix in this ctor).
  // Since the Lagrange basis is a partition of unity (\sum_i phi_i == 1, hence \sum_i \nabla phi_i == 0), we have
  //     \sum_i local_dof_i = -\int_E f(u) . \nabla( \sum_i phi_i ) = -\int_E f(u) . \nabla 1 = 0
  // for any source and any flux. We assert this exact structural invariant and require the update to be non-trivial.
  void volume_operator_updates_sum_to_zero()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto dg_space = make_discontinuous_lagrange_space(grid_view, 1);
    auto source = make_discrete_function<V>(dg_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, 1.); // constant field u == 1
    auto flux = make_flux();
    NumericalFluxType numerical_flux(flux);
    VolumeOperatorType op(source, numerical_flux.flux());
    const auto element = *(grid_view.template begin<0>());
    auto range = make_discrete_function<V>(dg_space);
    auto local_range = range.local_discrete_function();
    local_range->bind(element);
    op.bind(element);
    op.apply(*local_range);
    double sum = 0.;
    double max_abs = 0.;
    for (size_t ii = 0; ii < local_range->dofs().size(); ++ii) {
      const double v = local_range->dofs().get_entry(ii);
      sum += v;
      max_abs = std::max(max_abs, std::abs(v));
    }
    EXPECT_LT(std::abs(sum), 1e-10);
    EXPECT_GT(max_abs, 0.);
  }

  void volume_operator_copy_yields_the_same_result()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto dg_space = make_discontinuous_lagrange_space(grid_view, 1);
    auto source = make_discrete_function<V>(dg_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, 1.);
    auto flux = make_flux();
    NumericalFluxType numerical_flux(flux);
    VolumeOperatorType op(source, numerical_flux.flux());
    auto clone = op.copy();
    const auto element = *(grid_view.template begin<0>());
    auto range_a = make_discrete_function<V>(dg_space);
    auto range_b = make_discrete_function<V>(dg_space);
    auto lr_a = range_a.local_discrete_function();
    auto lr_b = range_b.local_discrete_function();
    lr_a->bind(element);
    lr_b->bind(element);
    op.bind(element);
    op.apply(*lr_a);
    clone->bind(element);
    clone->apply(*lr_b);
    for (size_t ii = 0; ii < lr_a->dofs().size(); ++ii)
      EXPECT_DOUBLE_EQ(lr_a->dofs().get_entry(ii), lr_b->dofs().get_entry(ii));
  }

  // The DG coupling operator adds  inside_dof_i  += \int_I g phi_i^in  and  outside_dof_i -= \int_I g phi_i^out,
  // with the *same* numerical flux g. By partition of unity (\sum_i phi_i == 1) this gives
  //     \sum_i inside_dof_i =  \int_I g  =  -\sum_i outside_dof_i,
  // i.e. the operator is locally conservative across the intersection, for any source and any flux. We assert this.
  void coupling_operator_is_locally_conservative()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto dg_space = make_discontinuous_lagrange_space(grid_view, 1);
    auto source = make_discrete_function<V>(dg_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, 1.);
    auto flux = make_flux();
    NumericalFluxType numerical_flux(flux);
    CouplingOperatorType op(source, numerical_flux);
    const auto element = *(grid_view.template begin<0>());
    double max_abs_inside_sum = 0.;
    for (auto&& intersection : intersections(grid_view, element)) {
      if (!intersection.neighbor())
        continue;
      auto range = make_discrete_function<V>(dg_space);
      auto local_range_inside = range.local_discrete_function();
      auto local_range_outside = range.local_discrete_function();
      local_range_inside->bind(intersection.inside());
      local_range_outside->bind(intersection.outside());
      op.bind(intersection);
      op.apply(*local_range_inside, *local_range_outside);
      double inside_sum = 0.;
      for (size_t ii = 0; ii < local_range_inside->dofs().size(); ++ii)
        inside_sum += local_range_inside->dofs().get_entry(ii);
      double outside_sum = 0.;
      for (size_t ii = 0; ii < local_range_outside->dofs().size(); ++ii)
        outside_sum += local_range_outside->dofs().get_entry(ii);
      EXPECT_NEAR(inside_sum, -1. * outside_sum, 1e-12);
      max_abs_inside_sum = std::max(max_abs_inside_sum, std::abs(inside_sum));
    }
    EXPECT_GT(max_abs_inside_sum, 0.);
  }

  void coupling_operator_copy_yields_the_same_result()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto dg_space = make_discontinuous_lagrange_space(grid_view, 1);
    auto source = make_discrete_function<V>(dg_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, 1.);
    auto flux = make_flux();
    NumericalFluxType numerical_flux(flux);
    CouplingOperatorType op(source, numerical_flux);
    auto clone = op.copy();
    const auto element = *(grid_view.template begin<0>());
    for (auto&& intersection : intersections(grid_view, element)) {
      if (!intersection.neighbor())
        continue;
      auto range_a = make_discrete_function<V>(dg_space);
      auto range_b = make_discrete_function<V>(dg_space);
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
      for (size_t ii = 0; ii < lr_a_in->dofs().size(); ++ii)
        EXPECT_DOUBLE_EQ(lr_a_in->dofs().get_entry(ii), lr_b_in->dofs().get_entry(ii));
      for (size_t ii = 0; ii < lr_a_out->dofs().size(); ++ii)
        EXPECT_DOUBLE_EQ(lr_a_out->dofs().get_entry(ii), lr_b_out->dofs().get_entry(ii));
    }
  }
}; // struct LocalAdvectionDgOperatorTest


template <class G>
using LocalAdvectionDgOperatorTests = LocalAdvectionDgOperatorTest<G>;
TYPED_TEST_SUITE(LocalAdvectionDgOperatorTests, Grids2D);
TYPED_TEST(LocalAdvectionDgOperatorTests, volume_operator_updates_sum_to_zero)
{
  this->volume_operator_updates_sum_to_zero();
}
TYPED_TEST(LocalAdvectionDgOperatorTests, volume_operator_copy_yields_the_same_result)
{
  this->volume_operator_copy_yields_the_same_result();
}
TYPED_TEST(LocalAdvectionDgOperatorTests, coupling_operator_is_locally_conservative)
{
  this->coupling_operator_is_locally_conservative();
}
TYPED_TEST(LocalAdvectionDgOperatorTests, coupling_operator_copy_yields_the_same_result)
{
  this->coupling_operator_copy_yields_the_same_result();
}
