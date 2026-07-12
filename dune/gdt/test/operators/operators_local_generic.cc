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
#include <dune/xt/common/parameter.hh>
#include <dune/xt/common/string.hh>
#include <dune/xt/la/container/istl.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/discretefunction/default.hh>
#include <dune/gdt/local/operators/generic.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>

#include <dune/gdt/test/integrands/integrands.hh> // <- provides the Grids2D type list and the DXTC typenames

using namespace Dune;
using namespace Dune::GDT;


template <class G>
struct GenericLocalElementOperatorTest : public ::testing::Test
{
  static constexpr size_t d = G::dimension;
  using GV = typename G::LeafGridView;
  using E = XT::Grid::extract_entity_t<GV>;
  using V = XT::LA::IstlDenseVector<double>;
  using GP = XT::Grid::GridProvider<G>;
  using OperatorType = GenericLocalElementOperator<V, GV>;
  using GenericFunctionType = typename OperatorType::GenericFunctionType;

  static GP make_grid()
  {
    return GP(XT::Grid::make_cube_grid<G>(XT::Common::from_string<FieldVector<double, d>>("[0 0 0 0]"),
                                          XT::Common::from_string<FieldVector<double, d>>("[1 1 1 1]"),
                                          XT::Common::from_string<std::array<unsigned int, d>>("[2 2 2 2]")));
  }

  // The generic operator simply forwards to the user provided lambda. We check that the lambda is actually
  // invoked, that it receives the (bound) local source and that whatever it writes into the local range ends up
  // in the range DoFs. The lambda writes factor * u into the single FV range DoF.
  void forwards_to_the_lambda_and_modifies_the_range()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto fv_space = make_finite_volume_space(grid_view);
    const double c = 2.5;
    auto source = make_discrete_function<V>(fv_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, c);
    bool lambda_was_called = false;
    const double factor = 3.;
    GenericFunctionType func =
        [&lambda_was_called,
         factor](const auto& /*source*/, const auto& local_sources, auto& local_range, const auto& /*param*/) {
          lambda_was_called = true;
          const auto& element = local_range.element();
          const auto x_in_reference_element = element.geometry().local(element.geometry().center());
          const auto u = local_sources[0]->evaluate(x_in_reference_element)[0];
          local_range.dofs()[0] = factor * u;
        };
    OperatorType op(source, func, /*num_local_sources=*/1);
    const auto element = *(grid_view.template begin<0>());
    auto range = make_discrete_function<V>(fv_space);
    auto local_range = range.local_discrete_function();
    local_range->bind(element);
    op.bind(element);
    op.apply(*local_range);
    EXPECT_TRUE(lambda_was_called);
    EXPECT_DOUBLE_EQ(factor * c, local_range->dofs().get_entry(0));
  }

  void propagates_the_parameter_type()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto fv_space = make_finite_volume_space(grid_view);
    auto source = make_discrete_function<V>(fv_space);
    GenericFunctionType func =
        [](const auto& /*source*/, const auto& /*local_sources*/, auto& /*local_range*/, const auto& /*param*/) {};
    const XT::Common::ParameterType param_type("mu", 1);
    OperatorType op(source, func, /*num_local_sources=*/1, param_type);
    EXPECT_TRUE(op.parameter_type() == param_type);
  }

  void copy_yields_the_same_result()
  {
    auto grid_provider = make_grid();
    auto grid_view = grid_provider.leaf_view();
    auto fv_space = make_finite_volume_space(grid_view);
    const double c = 4.;
    auto source = make_discrete_function<V>(fv_space);
    for (size_t ii = 0; ii < source.dofs().vector().size(); ++ii)
      source.dofs().vector().set_entry(ii, c);
    const double factor = 2.;
    GenericFunctionType func =
        [factor](const auto& /*source*/, const auto& local_sources, auto& local_range, const auto& /*param*/) {
          const auto& element = local_range.element();
          const auto x_in_reference_element = element.geometry().local(element.geometry().center());
          local_range.dofs()[0] = factor * local_sources[0]->evaluate(x_in_reference_element)[0];
        };
    OperatorType op(source, func, /*num_local_sources=*/1);
    const auto element = *(grid_view.template begin<0>());
    auto clone = op.copy();
    auto range = make_discrete_function<V>(fv_space);
    auto local_range = range.local_discrete_function();
    local_range->bind(element);
    clone->bind(element);
    clone->apply(*local_range);
    EXPECT_DOUBLE_EQ(factor * c, local_range->dofs().get_entry(0));
  }
}; // struct GenericLocalElementOperatorTest


template <class G>
using GenericLocalElementOperatorTests = GenericLocalElementOperatorTest<G>;
TYPED_TEST_SUITE(GenericLocalElementOperatorTests, Grids2D);
TYPED_TEST(GenericLocalElementOperatorTests, forwards_to_the_lambda_and_modifies_the_range)
{
  this->forwards_to_the_lambda_and_modifies_the_range();
}
TYPED_TEST(GenericLocalElementOperatorTests, propagates_the_parameter_type)
{
  this->propagates_the_parameter_type();
}
TYPED_TEST(GenericLocalElementOperatorTests, copy_yields_the_same_result)
{
  this->copy_yields_the_same_result();
}
