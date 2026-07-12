// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/string.hh>
#include <dune/xt/la/container/istl.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/discretefunction/default.hh>
#include <dune/gdt/local/bilinear-forms/integrals.hh>
#include <dune/gdt/local/integrands/product.hh>
#include <dune/gdt/local/operators/indicator.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>

#include <dune/gdt/test/integrands/integrands.hh> // <- provides the Grids2D type list and the DXTC typenames

using namespace Dune;
using namespace Dune::GDT;


template <class G>
struct LocalElementBilinearFormIndicatorOperatorTest : public ::testing::Test
{
  static constexpr size_t d = G::dimension;
  using GV = typename G::LeafGridView;
  using E = XT::Grid::extract_entity_t<GV>;
  using V = XT::LA::IstlDenseVector<double>;
  using GP = XT::Grid::GridProvider<G>;

  // The local L2 product bilinear form gives, applied to the source u with itself, \int_E u^2 dx.
  // For a piecewise constant (FV) source with value c on the element E this is exactly c^2 * |E|.
  using BilinearFormType = LocalElementIntegralBilinearForm<E>;
  using OperatorType = LocalElementBilinearFormIndicatorOperator<V, GV>;

  static GP make_grid()
  {
    return GP(XT::Grid::make_cube_grid<G>(XT::Common::from_string<FieldVector<double, d>>("[0 0 0 0]"),
                                          XT::Common::from_string<FieldVector<double, d>>("[1 1 1 1]"),
                                          XT::Common::from_string<std::array<unsigned int, d>>("[2 2 2 2]")));
  }

  void computes_the_local_l2_norm_squared()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto fv_space = make_finite_volume_space(grid_view);
    const double c = 2.;
    auto source = make_discrete_function<V>(fv_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, c);
    BilinearFormType bilinear_form(LocalProductIntegrand<E>());
    OperatorType op(bilinear_form, source);
    const auto element = *(grid_view.template begin<0>());
    auto range = make_discrete_function<V>(fv_space);
    auto local_range = range.local_discrete_function();
    local_range->bind(element);
    op.bind(element);
    op.apply(*local_range);
    const double expected = c * c * element.geometry().volume();
    EXPECT_DOUBLE_EQ(expected, local_range->dofs().get_entry(0));
  }

  void copy_yields_the_same_result()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto fv_space = make_finite_volume_space(grid_view);
    const double c = 1.5;
    auto source = make_discrete_function<V>(fv_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, c);
    BilinearFormType bilinear_form(LocalProductIntegrand<E>());
    OperatorType op(bilinear_form, source);
    const auto element = *(grid_view.template begin<0>());
    auto range = make_discrete_function<V>(fv_space);
    auto local_range = range.local_discrete_function();
    local_range->bind(element);
    op.bind(element);
    op.apply(*local_range);
    // clone via the polymorphic copy() and re-apply on a fresh range
    auto clone = op.copy();
    auto range_clone = make_discrete_function<V>(fv_space);
    auto local_range_clone = range_clone.local_discrete_function();
    local_range_clone->bind(element);
    clone->bind(element);
    clone->apply(*local_range_clone);
    EXPECT_DOUBLE_EQ(local_range->dofs().get_entry(0), local_range_clone->dofs().get_entry(0));
  }

  void with_source_after_default_construction_yields_the_same_result()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto fv_space = make_finite_volume_space(grid_view);
    const double c = 3.;
    auto source = make_discrete_function<V>(fv_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, c);
    BilinearFormType bilinear_form(LocalProductIntegrand<E>());
    // construct without source, then set it via with_source
    OperatorType op_without_source(bilinear_form);
    auto op = op_without_source.with_source(source);
    const auto element = *(grid_view.template begin<0>());
    auto range = make_discrete_function<V>(fv_space);
    auto local_range = range.local_discrete_function();
    local_range->bind(element);
    op->bind(element);
    op->apply(*local_range);
    const double expected = c * c * element.geometry().volume();
    EXPECT_DOUBLE_EQ(expected, local_range->dofs().get_entry(0));
  }
}; // struct LocalElementBilinearFormIndicatorOperatorTest


template <class G>
using LocalIndicatorOperator = LocalElementBilinearFormIndicatorOperatorTest<G>;
TYPED_TEST_SUITE(LocalIndicatorOperator, Grids2D);
TYPED_TEST(LocalIndicatorOperator, computes_the_local_l2_norm_squared)
{
  this->computes_the_local_l2_norm_squared();
}
TYPED_TEST(LocalIndicatorOperator, copy_yields_the_same_result)
{
  this->copy_yields_the_same_result();
}
TYPED_TEST(LocalIndicatorOperator, with_source_after_default_construction_yields_the_same_result)
{
  this->with_source_after_default_construction_yields_the_same_result();
}
