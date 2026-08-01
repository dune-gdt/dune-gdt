// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2018)
//   René Fritze     (2018)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/common/dynvector.hh>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/fvector.hh>
#include <dune/xt/functions/generic/element-function.hh>
#include <dune/xt/functions/interfaces/element-functions.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/exceptions.hh>
#include <dune/gdt/spaces/basis/interface.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
static constexpr size_t d = G::dimension;
using D = typename G::ctype;
using R = double;


GTEST_TEST(spaces_basis_finite_volume, basis_has_size_one_and_order_zero)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);

  for (auto&& element : Dune::elements(grid_view)) {
    const auto basis = space.basis().localize(element);
    EXPECT_EQ(1u, basis->size());
    EXPECT_EQ(0, basis->order());
  }
}


// Mirrors basis_is_finite_volume_basis from spaces/base.hh: the single constant basis function evaluates to 1
// everywhere.
GTEST_TEST(spaces_basis_finite_volume, basis_evaluates_to_one)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);

  const double tolerance = 1e-15;
  for (auto&& element : Dune::elements(grid_view)) {
    const auto values = space.basis().localize(element)->evaluate_set(FieldVector<D, d>(0.));
    ASSERT_EQ(1u, values.size());
    EXPECT_TRUE(XT::Common::FloatCmp::eq(values.at(0), FieldVector<R, 1>(1.), tolerance, tolerance));
  }
}


// The single basis function is constant, hence its jacobian vanishes everywhere.
GTEST_TEST(spaces_basis_finite_volume, basis_jacobians_vanish)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);

  const double tolerance = 1e-15;
  for (auto&& element : Dune::elements(grid_view)) {
    const auto grads = space.basis().localize(element)->jacobians_of_set(FieldVector<D, d>(0.));
    ASSERT_EQ(1u, grads.size());
    EXPECT_TRUE(XT::Common::FloatCmp::eq(grads.at(0), decltype(grads[0])(0), tolerance, tolerance));
  }
}


// max_size is the number of basis functions any element can carry, which for a scalar FV space is one -- on the
// global basis (which reports it without being bound) as well as on the localized view.
GTEST_TEST(spaces_basis_finite_volume, max_size_is_one)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);

  EXPECT_EQ(1u, space.basis().max_size());
  const auto element = *Dune::elements(grid_view).begin();
  EXPECT_EQ(1u, space.basis().localize(element)->max_size());
}


// derivatives() answers the all-zero multi-index (regression test: the zeroth-order branch used to index the rows of
// the r x d derivative range by the direction jj, out of bounds from the second axis on for a scalar basis) and
// rejects any actual (non-zero) derivative order. alpha = (0, 1) walks the axis loop through both of its branches:
// the zeroth-order assignment for the first axis, then the documented rejection for the second.
GTEST_TEST(spaces_basis_finite_volume, derivatives_reject_nonzero_orders)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const auto element = *Dune::elements(grid_view).begin();
  const auto basis = space.basis().localize(element);

  using FunctionSet =
      XT::Functions::ElementFunctionSetInterface<XT::Grid::extract_entity_t<decltype(grid_view)>, 1, 1, R>;
  std::vector<typename FunctionSet::DerivativeRangeType> result(1);
  basis->derivatives({{0, 0}}, FieldVector<D, d>(0.), result);
  ASSERT_GE(result.size(), 1u);
  for (size_t jj = 0; jj < d; ++jj)
    EXPECT_DOUBLE_EQ(1., result[0][0][jj]);
  EXPECT_THROW(basis->derivatives({{0, 1}}, FieldVector<D, d>(0.), result), Exceptions::basis_error);
}


// The performance overrides for dynamically-sized and single-component results must agree with the statically-sized
// evaluate/jacobians: the constant one and its vanishing gradient. Passing empty result vectors additionally runs
// their resize branches.
GTEST_TEST(spaces_basis_finite_volume, dynamic_and_single_component_variants_agree)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const auto element = *Dune::elements(grid_view).begin();
  const auto basis = space.basis().localize(element);
  const FieldVector<D, d> point(0.);

  using FunctionSet =
      XT::Functions::ElementFunctionSetInterface<XT::Grid::extract_entity_t<decltype(grid_view)>, 1, 1, R>;
  std::vector<typename FunctionSet::DynamicRangeType> dynamic_values;
  basis->evaluate(point, dynamic_values);
  ASSERT_GE(dynamic_values.size(), 1u);
  EXPECT_DOUBLE_EQ(1., dynamic_values[0][0]);

  std::vector<typename FunctionSet::DynamicDerivativeRangeType> dynamic_grads;
  basis->jacobians(point, dynamic_grads);
  ASSERT_GE(dynamic_grads.size(), 1u);

  std::vector<R> component_values;
  basis->evaluate(point, component_values, /*row=*/0, /*col=*/0);
  ASSERT_GE(component_values.size(), 1u);
  EXPECT_DOUBLE_EQ(1., component_values[0]);

  std::vector<typename FunctionSet::SingleDerivativeRangeType> component_grads;
  basis->jacobians(point, component_grads, /*row=*/0, /*col=*/0);
  ASSERT_GE(component_grads.size(), 1u);
  EXPECT_TRUE(XT::Common::FloatCmp::eq(component_grads[0], typename FunctionSet::SingleDerivativeRangeType(0.)));
}


// Regression test: the dynamically-sized evaluate wrote the constant into result[ii] (the vector of basis functions,
// which has length one) instead of result[0][ii] (the components of the single basis function), reading/writing out
// of bounds for every vector-valued FV space.
GTEST_TEST(spaces_basis_finite_volume, dynamic_evaluate_fills_all_components_of_a_vector_valued_basis)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space<2>(grid_view);
  const auto element = *Dune::elements(grid_view).begin();
  const auto basis = space.basis().localize(element);

  using FunctionSet =
      XT::Functions::ElementFunctionSetInterface<XT::Grid::extract_entity_t<decltype(grid_view)>, 2, 1, R>;
  std::vector<typename FunctionSet::DynamicRangeType> dynamic_values;
  basis->evaluate(FieldVector<D, d>(0.), dynamic_values);
  ASSERT_GE(dynamic_values.size(), 1u);
  ASSERT_GE(dynamic_values[0].size(), 2u);
  EXPECT_DOUBLE_EQ(1., dynamic_values[0][0]);
  EXPECT_DOUBLE_EQ(1., dynamic_values[0][1]);
}


// The FV basis carries no per-element FE data: default_data and backup are empty, restore accepts exactly that and
// rejects anything else, and finite_element() hands out the underlying (P0 Lagrange) finite element.
GTEST_TEST(spaces_basis_finite_volume, backup_restore_and_finite_element)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const auto element = *Dune::elements(grid_view).begin();
  auto basis = space.basis().localize(element);

  EXPECT_EQ(0u, basis->default_data(element.type()).size());
  EXPECT_EQ(0u, basis->backup().size());
  EXPECT_EQ(1u, basis->finite_element().size());
  basis->restore(element, DynamicVector<R>());
  EXPECT_THROW(basis->restore(element, DynamicVector<R>(1, 0.)), Exceptions::finite_element_error);
}


// Interpolation into the FV basis is the element average; a pre-sized DoF vector of the wrong length must be resized
// (the resize branch of interpolate), and the DynamicVector-returning convenience overload of the localized-basis
// interface (dune/gdt/spaces/basis/interface.hh) must yield the same single value.
GTEST_TEST(spaces_basis_finite_volume, interpolate_resizes_and_convenience_overloads_agree)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const auto element = *Dune::elements(grid_view).begin();
  const auto basis = space.basis().localize(element);
  const auto constant_two = [](const auto& /*point*/) { return FieldVector<R, 1>(2.); };

  DynamicVector<R> wrongly_sized_dofs(3, 0.);
  basis->interpolate(constant_two, /*order=*/0, wrongly_sized_dofs);
  ASSERT_EQ(1u, wrongly_sized_dofs.size());
  EXPECT_DOUBLE_EQ(2., wrongly_sized_dofs[0]);

  const auto returned_dofs = basis->interpolate(constant_two, /*order=*/0);
  ASSERT_EQ(1u, returned_dofs.size());
  EXPECT_DOUBLE_EQ(2., returned_dofs[0]);

  // the ElementFunctionInterface-taking convenience forwards to the std::function variant
  XT::Functions::GenericElementFunction<XT::Grid::extract_entity_t<decltype(grid_view)>, 1, 1, R> two_as_function(
      /*order=*/0, [](const auto& /*point*/, const auto& /*param*/) { return FieldVector<R, 1>(2.); });
  two_as_function.bind(element);
  DynamicVector<R> dofs_from_function;
  basis->interpolate(two_as_function, dofs_from_function);
  ASSERT_EQ(1u, dofs_from_function.size());
  EXPECT_DOUBLE_EQ(2., dofs_from_function[0]);
}


// A basis that overrides only the pure-virtual localize() runs the *default* implementations of
// GlobalBasisInterface: max_size() localizes and asks the localized view, update_after_adapt() reports that
// adaptation is unsupported. The stub forwards localize() to a real FV basis so the defaults have something to
// work with.
GTEST_TEST(spaces_basis_finite_volume, global_basis_interface_defaults)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);

  using GV = decltype(grid_view);
  struct MinimalBasis : GlobalBasisInterface<GV>
  {
    MinimalBasis(const GlobalBasisInterface<GV>& wrapped)
      : wrapped_(wrapped)
    {
    }
    std::unique_ptr<typename GlobalBasisInterface<GV>::LocalizedType> localize() const override
    {
      return wrapped_.localize();
    }
    const GlobalBasisInterface<GV>& wrapped_;
  };

  MinimalBasis minimal_basis(space.basis());
  EXPECT_EQ(space.basis().max_size(), minimal_basis.max_size());
  EXPECT_THROW(minimal_basis.update_after_adapt(), Exceptions::basis_error);
}
