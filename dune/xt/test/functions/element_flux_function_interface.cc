// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-xt developers

/// \file
/// \brief Tests the default implementations provided by the flux function interfaces.
///
/// Covers ElementFluxFunctionSetInterface, ElementFluxFunctionInterface (which is a set of size one) and the small
/// FluxFunctionInterface on top of them. As for the plain element functions, concrete implementations override the
/// "should be implemented" virtuals, so the convenience layers the interfaces provide -- the single-component
/// accessors, the dynamic-container overloads, the argument checks and the NotImplemented defaults -- are only
/// reachable from subclasses that deliberately leave them alone, which is what the minimal classes below do.
///
/// \note Everything here uses a state dimension equal to the domain dimension. The interfaces size their jacobian
///       types from the state dimension (JacobianRangeSelector is a DerivativeRangeTypeSelector<s, ...>) but iterate
///       over the domain dimension d in the matrix-valued branches of the single-component accessors, so the two have
///       to agree for those code paths to be well defined.

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- has to come first, includes the config.h!

#include <memory>
#include <string>
#include <vector>

#include <dune/xt/test/functions/interface_fixture.hh>

#include <dune/xt/functions/exceptions.hh>
#include <dune/xt/functions/interfaces/element-flux-functions.hh>
#include <dune/xt/functions/interfaces/flux-function.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::Functions;
using namespace Dune::XT::Test;

namespace {


using ElementType = FunctionInterfaceTestBase::ElementType;
constexpr size_t dim_domain = FunctionInterfaceTestBase::dim_domain;
constexpr size_t dim_state = dim_domain;

const FieldVector<double, dim_state> state(0.5);


/// \brief Implements only the three pure virtuals, so every "should be implemented" method still throws.
template <size_t r, size_t rC>
class ThrowingSet : public ElementFluxFunctionSetInterface<ElementType, dim_state, r, rC>
{
public:
  size_t size(const Common::Parameter& /*param*/ = {}) const override
  {
    return 1;
  }

  size_t max_size(const Common::Parameter& /*param*/ = {}) const override
  {
    return 1;
  }

  int order(const Common::Parameter& /*param*/ = {}) const override
  {
    return 0;
  }
};


/// \brief Implements only order(), so evaluate()/jacobian() still throw.
template <size_t r, size_t rC>
class ThrowingLocalFunction : public ElementFluxFunctionInterface<ElementType, dim_state, r, rC>
{
public:
  int order(const Common::Parameter& /*param*/ = {}) const override
  {
    return 0;
  }
};


/// \brief Implements the two single-valued methods only; everything else is served by the interfaces.
template <size_t r, size_t rC>
class FixedLocalFunction : public ElementFluxFunctionInterface<ElementType, dim_state, r, rC>
{
  using BaseType = ElementFluxFunctionInterface<ElementType, dim_state, r, rC>;

public:
  using BaseType::s;
  using typename BaseType::DomainType;
  using typename BaseType::JacobianRangeReturnType;
  using typename BaseType::RangeReturnType;
  using typename BaseType::StateType;

  // overriding one overload hides the others, and those are exactly what is under test here
  using BaseType::evaluate;
  using BaseType::jacobian;

  int order(const Common::Parameter& /*param*/ = {}) const override
  {
    return 0;
  }

  RangeReturnType evaluate(const DomainType& point_in_reference_element,
                           const StateType& /*u*/,
                           const Common::Parameter& /*param*/ = {}) const override
  {
    this->assert_inside_reference_element(point_in_reference_element);
    return filled_range<r, rC, RangeReturnType>(0);
  }

  JacobianRangeReturnType jacobian(const DomainType& point_in_reference_element,
                                   const StateType& /*u*/,
                                   const Common::Parameter& /*param*/ = {}) const override
  {
    this->assert_inside_reference_element(point_in_reference_element);
    return filled_derivative<r, rC, s, JacobianRangeReturnType>(0);
  }
};


/// \brief Implements only local_function(), so x_dependent() and name() fall back to the interface defaults.
template <size_t r, size_t rC>
class MinimalFluxFunction : public FluxFunctionInterface<ElementType, dim_state, r, rC>
{
  using BaseType = FluxFunctionInterface<ElementType, dim_state, r, rC>;

public:
  using typename BaseType::LocalFunctionType;

  std::unique_ptr<LocalFunctionType> local_function() const override
  {
    return std::make_unique<FixedLocalFunction<r, rC>>();
  }
};


struct FluxFunctionInterfaceTest : public FunctionInterfaceTestBase
{};


// ======================================================================== ElementFluxFunctionSetInterface


template <size_t r, size_t rC>
void check_set_defaults_throw(const ElementType& element)
{
  ThrowingSet<r, rC> set;
  set.bind(element);

  std::vector<typename ThrowingSet<r, rC>::RangeType> values;
  std::vector<typename ThrowingSet<r, rC>::JacobianRangeType> jacobians;

  EXPECT_THROW(set.evaluate(inside_point(), state, values), Dune::NotImplemented);
  EXPECT_THROW(set.jacobian(inside_point(), state, jacobians), Dune::NotImplemented);
  // the convenience wrappers forward to the same defaults
  EXPECT_THROW(set.evaluate_set(inside_point(), state), Dune::NotImplemented);
  EXPECT_THROW(set.jacobian_of_set(inside_point(), state), Dune::NotImplemented);
} // ... check_set_defaults_throw(...)


template <size_t r, size_t rC>
void check_of_set_wrappers(const ElementType& element)
{
  FixedLocalFunction<r, rC> func;
  func.bind(element);

  // an ElementFluxFunctionInterface is an ElementFluxFunctionSetInterface of size one
  EXPECT_EQ(1u, func.size());
  EXPECT_EQ(1u, func.max_size());

  const auto values = func.evaluate_set(inside_point(), state);
  ASSERT_EQ(1u, values.size());
  expect_range_eq<r, rC>(values[0], 0);

  const auto jacobians = func.jacobian_of_set(inside_point(), state);
  ASSERT_EQ(1u, jacobians.size());
  expect_derivative_eq<r, rC, dim_state>(jacobians[0], 0);
} // ... check_of_set_wrappers(...)


template <size_t r, size_t rC>
void check_set_single_component_accessors(const ElementType& element)
{
  FixedLocalFunction<r, rC> func;
  func.bind(element);
  using SingleJacobianRangeType = typename FixedLocalFunction<r, rC>::SingleJacobianRangeType;

  for (size_t row = 0; row < r; ++row)
    for (size_t col = 0; col < rC; ++col) {
      // both results are deliberately left empty, to also cover the interface's resize path
      std::vector<double> values;
      func.evaluate(inside_point(), state, values, row, col);
      ASSERT_EQ(1u, values.size());
      EXPECT_DOUBLE_EQ(expected_value(0, row, col), values[0]);

      std::vector<SingleJacobianRangeType> jacobians;
      func.jacobian(inside_point(), state, jacobians, row, col);
      ASSERT_EQ(1u, jacobians.size());
      for (size_t ss = 0; ss < dim_state; ++ss)
        EXPECT_DOUBLE_EQ(expected_derivative(0, row, col, ss), jacobians[0][ss]);
    }
} // ... check_set_single_component_accessors(...)


template <size_t r, size_t rC>
void check_set_dynamic_overloads(const ElementType& element)
{
  FixedLocalFunction<r, rC> func;
  func.bind(element);
  using DynamicRangeType = typename FixedLocalFunction<r, rC>::DynamicRangeType;
  using DynamicJacobianRangeType = typename FixedLocalFunction<r, rC>::DynamicJacobianRangeType;

  // both results are deliberately left empty, so the interface has to resize and ensure_size the entries
  std::vector<DynamicRangeType> values;
  func.evaluate(inside_point(), state, values);
  ASSERT_EQ(1u, values.size());
  expect_dynamic_range_eq<r, rC>(values[0], 0);

  std::vector<DynamicJacobianRangeType> jacobians;
  func.jacobian(inside_point(), state, jacobians);
  ASSERT_EQ(1u, jacobians.size());
  expect_dynamic_derivative_eq<r, rC, dim_state>(jacobians[0], 0);
} // ... check_set_dynamic_overloads(...)


template <size_t r, size_t rC>
void check_set_argument_checks(const ElementType& element)
{
  FixedLocalFunction<r, rC> func;
  func.bind(element);
  using SingleJacobianRangeType = typename FixedLocalFunction<r, rC>::SingleJacobianRangeType;

  std::vector<double> values;
  std::vector<SingleJacobianRangeType> jacobians;

  EXPECT_THROW(func.evaluate(inside_point(), state, values, /*row=*/r, /*col=*/0),
               Common::Exceptions::shapes_do_not_match);
  EXPECT_THROW(func.jacobian(inside_point(), state, jacobians, /*row=*/0, /*col=*/rC),
               Common::Exceptions::shapes_do_not_match);

  // assert_inside_reference_element() is what FixedLocalFunction calls at the top of evaluate()/jacobian()
  EXPECT_THROW(func.evaluate(outside_point(), state), Functions::Exceptions::wrong_input_given);
} // ... check_set_argument_checks(...)


TEST_F(FluxFunctionInterfaceTest, set_defaults_throw)
{
  check_set_defaults_throw<2, 1>(element_);
  check_set_defaults_throw<2, 3>(element_);
}

TEST_F(FluxFunctionInterfaceTest, of_set_wrappers)
{
  check_of_set_wrappers<2, 1>(element_);
  check_of_set_wrappers<2, 3>(element_);
}

TEST_F(FluxFunctionInterfaceTest, set_single_component_accessors)
{
  check_set_single_component_accessors<2, 1>(element_);
  check_set_single_component_accessors<2, 3>(element_);
}

TEST_F(FluxFunctionInterfaceTest, set_dynamic_overloads)
{
  check_set_dynamic_overloads<2, 1>(element_);
  check_set_dynamic_overloads<2, 3>(element_);
}

TEST_F(FluxFunctionInterfaceTest, set_argument_checks)
{
  check_set_argument_checks<2, 1>(element_);
  check_set_argument_checks<2, 3>(element_);
}


// =========================================================================== ElementFluxFunctionInterface


template <size_t r, size_t rC>
void check_local_function_defaults_throw(const ElementType& element)
{
  ThrowingLocalFunction<r, rC> func;
  func.bind(element);

  EXPECT_THROW(func.evaluate(inside_point(), state), Dune::NotImplemented);
  EXPECT_THROW(func.jacobian(inside_point(), state), Dune::NotImplemented);
} // ... check_local_function_defaults_throw(...)


template <size_t r, size_t rC>
void check_local_function_single_component_accessors(const ElementType& element)
{
  FixedLocalFunction<r, rC> func;
  func.bind(element);

  for (size_t row = 0; row < r; ++row)
    for (size_t col = 0; col < rC; ++col) {
      EXPECT_DOUBLE_EQ(expected_value(0, row, col), func.evaluate(inside_point(), state, row, col));

      const auto jacobian = func.jacobian(inside_point(), state, row, col);
      for (size_t ss = 0; ss < dim_state; ++ss)
        EXPECT_DOUBLE_EQ(expected_derivative(0, row, col, ss), jacobian[ss]);
    }
} // ... check_local_function_single_component_accessors(...)


template <size_t r, size_t rC>
void check_local_function_dynamic_overloads(const ElementType& element)
{
  using FunctionType = FixedLocalFunction<r, rC>;
  FunctionType func;
  func.bind(element);

  // both results are default constructed, so the interface has to ensure_size them
  typename FunctionType::DynamicRangeType values;
  func.evaluate(inside_point(), state, values);
  expect_dynamic_range_eq<r, rC>(values, 0);

  typename FunctionType::DynamicJacobianRangeType jacobian;
  func.jacobian(inside_point(), state, jacobian);
  expect_dynamic_derivative_eq<r, rC, dim_state>(jacobian, 0);
} // ... check_local_function_dynamic_overloads(...)


TEST_F(FluxFunctionInterfaceTest, local_function_defaults_throw)
{
  check_local_function_defaults_throw<2, 1>(element_);
  check_local_function_defaults_throw<2, 3>(element_);
}

TEST_F(FluxFunctionInterfaceTest, local_function_single_component_accessors)
{
  check_local_function_single_component_accessors<2, 1>(element_);
  check_local_function_single_component_accessors<2, 3>(element_);
}

TEST_F(FluxFunctionInterfaceTest, local_function_dynamic_overloads)
{
  check_local_function_dynamic_overloads<2, 1>(element_);
  check_local_function_dynamic_overloads<2, 3>(element_);
}


// ==================================================================================== FluxFunctionInterface


template <size_t r, size_t rC>
void check_flux_function_defaults(const ElementType& element)
{
  const MinimalFluxFunction<r, rC> flux;

  // both are interface defaults a concrete flux function is expected to override
  EXPECT_TRUE(flux.x_dependent());
  EXPECT_EQ(std::string("dune.xt.functions.fluxfunction"), flux.name());

  auto local_function = flux.local_function();
  ASSERT_NE(local_function, nullptr);
  local_function->bind(element);
  EXPECT_DOUBLE_EQ(expected_value(0, 0, 0), local_function->evaluate(inside_point(), state, 0, 0));
} // ... check_flux_function_defaults(...)


TEST_F(FluxFunctionInterfaceTest, flux_function_defaults)
{
  check_flux_function_defaults<2, 1>(element_);
  check_flux_function_defaults<2, 3>(element_);
}


} // namespace
