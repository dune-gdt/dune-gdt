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

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/xt/functions/exceptions.hh>
#include <dune/xt/functions/interfaces/element-flux-functions.hh>
#include <dune/xt/functions/interfaces/flux-function.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::Functions;

namespace {


using GridType = CUBEGRID_2D;
using ElementType = XT::Grid::extract_entity_t<GridType>;
constexpr size_t dim_domain = GridType::dimension;
constexpr size_t dim_state = dim_domain;

// the reference element of a YaspGrid cube is [0, 1]^d
const FieldVector<double, dim_domain> inside_point(0.25);
const FieldVector<double, dim_domain> outside_point(1.5);
const FieldVector<double, dim_state> state(0.5);


/// \brief Deterministic, pairwise distinct values, so every accessed component can be identified in an assertion.
double expected_value(const size_t function_index, const size_t row, const size_t col)
{
  return 100. * function_index + 10. * row + col + 1.;
}

double expected_jacobian(const size_t function_index, const size_t row, const size_t col, const size_t ss)
{
  return 1000. * function_index + 100. * row + 10. * col + ss + 1.;
}


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
    RangeReturnType result;
    for (size_t row = 0; row < r; ++row) {
      if constexpr (rC == 1)
        result[row] = expected_value(0, row, 0);
      else
        for (size_t col = 0; col < rC; ++col)
          result[row][col] = expected_value(0, row, col);
    }
    return result;
  } // ... evaluate(...)

  JacobianRangeReturnType jacobian(const DomainType& point_in_reference_element,
                                   const StateType& /*u*/,
                                   const Common::Parameter& /*param*/ = {}) const override
  {
    this->assert_inside_reference_element(point_in_reference_element);
    JacobianRangeReturnType result;
    for (size_t row = 0; row < r; ++row) {
      if constexpr (rC == 1) {
        for (size_t ss = 0; ss < s; ++ss)
          result[row][ss] = expected_jacobian(0, row, 0, ss);
      } else {
        for (size_t col = 0; col < rC; ++col)
          for (size_t ss = 0; ss < s; ++ss)
            result[row][col][ss] = expected_jacobian(0, row, col, ss);
      }
    }
    return result;
  } // ... jacobian(...)
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


struct FluxFunctionInterfaceTest : public ::testing::Test
{
  using GridProviderType = XT::Grid::GridProvider<GridType>;

  FluxFunctionInterfaceTest()
    : grid_(XT::Grid::make_cube_grid<GridType>(0., 1., 2))
    , leaf_view_(grid_.leaf_view())
    // which element we bind to is irrelevant: everything under test here is element independent, it merely requires
    // the object to be bound at all
    , element_(*elements(leaf_view_).begin())
  {
  }

  const GridProviderType grid_;
  const typename GridProviderType::LeafGridViewType leaf_view_;
  const ElementType element_;
}; // struct FluxFunctionInterfaceTest


// ======================================================================== ElementFluxFunctionSetInterface


template <size_t r, size_t rC>
void check_set_defaults_throw(const ElementType& element)
{
  ThrowingSet<r, rC> set;
  set.bind(element);

  std::vector<typename ThrowingSet<r, rC>::RangeType> values;
  std::vector<typename ThrowingSet<r, rC>::JacobianRangeType> jacobians;

  EXPECT_THROW(set.evaluate(inside_point, state, values), Dune::NotImplemented);
  EXPECT_THROW(set.jacobian(inside_point, state, jacobians), Dune::NotImplemented);
  // the convenience wrappers forward to the same defaults
  EXPECT_THROW(set.evaluate_set(inside_point, state), Dune::NotImplemented);
  EXPECT_THROW(set.jacobian_of_set(inside_point, state), Dune::NotImplemented);
} // ... check_set_defaults_throw(...)


template <size_t r, size_t rC>
void check_of_set_wrappers(const ElementType& element)
{
  FixedLocalFunction<r, rC> func;
  func.bind(element);

  // an ElementFluxFunctionInterface is an ElementFluxFunctionSetInterface of size one
  EXPECT_EQ(1u, func.size());
  EXPECT_EQ(1u, func.max_size());

  const auto values = func.evaluate_set(inside_point, state);
  ASSERT_EQ(1u, values.size());
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1)
      EXPECT_DOUBLE_EQ(expected_value(0, row, 0), values[0][row]);
    else
      for (size_t col = 0; col < rC; ++col)
        EXPECT_DOUBLE_EQ(expected_value(0, row, col), values[0][row][col]);
  }

  const auto jacobians = func.jacobian_of_set(inside_point, state);
  ASSERT_EQ(1u, jacobians.size());
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1) {
      for (size_t ss = 0; ss < dim_state; ++ss)
        EXPECT_DOUBLE_EQ(expected_jacobian(0, row, 0, ss), jacobians[0][row][ss]);
    } else {
      for (size_t col = 0; col < rC; ++col)
        for (size_t ss = 0; ss < dim_state; ++ss)
          EXPECT_DOUBLE_EQ(expected_jacobian(0, row, col, ss), jacobians[0][row][col][ss]);
    }
  }
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
      func.evaluate(inside_point, state, values, row, col);
      ASSERT_EQ(1u, values.size());
      EXPECT_DOUBLE_EQ(expected_value(0, row, col), values[0]);

      std::vector<SingleJacobianRangeType> jacobians;
      func.jacobian(inside_point, state, jacobians, row, col);
      ASSERT_EQ(1u, jacobians.size());
      for (size_t ss = 0; ss < dim_state; ++ss)
        EXPECT_DOUBLE_EQ(expected_jacobian(0, row, col, ss), jacobians[0][ss]);
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
  func.evaluate(inside_point, state, values);
  ASSERT_EQ(1u, values.size());
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1)
      EXPECT_DOUBLE_EQ(expected_value(0, row, 0), values[0][row]);
    else
      for (size_t col = 0; col < rC; ++col)
        EXPECT_DOUBLE_EQ(expected_value(0, row, col), values[0].get_entry(row, col));
  }

  std::vector<DynamicJacobianRangeType> jacobians;
  func.jacobian(inside_point, state, jacobians);
  ASSERT_EQ(1u, jacobians.size());
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1) {
      for (size_t ss = 0; ss < dim_state; ++ss)
        EXPECT_DOUBLE_EQ(expected_jacobian(0, row, 0, ss), jacobians[0].get_entry(row, ss));
    } else {
      for (size_t col = 0; col < rC; ++col)
        for (size_t ss = 0; ss < dim_state; ++ss)
          EXPECT_DOUBLE_EQ(expected_jacobian(0, row, col, ss), jacobians[0][row].get_entry(col, ss));
    }
  }
} // ... check_set_dynamic_overloads(...)


template <size_t r, size_t rC>
void check_set_argument_checks(const ElementType& element)
{
  FixedLocalFunction<r, rC> func;
  func.bind(element);
  using SingleJacobianRangeType = typename FixedLocalFunction<r, rC>::SingleJacobianRangeType;

  std::vector<double> values;
  std::vector<SingleJacobianRangeType> jacobians;

  EXPECT_THROW(func.evaluate(inside_point, state, values, /*row=*/r, /*col=*/0),
               Common::Exceptions::shapes_do_not_match);
  EXPECT_THROW(func.jacobian(inside_point, state, jacobians, /*row=*/0, /*col=*/rC),
               Common::Exceptions::shapes_do_not_match);

  // assert_inside_reference_element() is what FixedLocalFunction calls at the top of evaluate()/jacobian()
  EXPECT_THROW(func.evaluate(outside_point, state), Functions::Exceptions::wrong_input_given);
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

  EXPECT_THROW(func.evaluate(inside_point, state), Dune::NotImplemented);
  EXPECT_THROW(func.jacobian(inside_point, state), Dune::NotImplemented);
} // ... check_local_function_defaults_throw(...)


template <size_t r, size_t rC>
void check_local_function_single_component_accessors(const ElementType& element)
{
  FixedLocalFunction<r, rC> func;
  func.bind(element);

  for (size_t row = 0; row < r; ++row)
    for (size_t col = 0; col < rC; ++col) {
      EXPECT_DOUBLE_EQ(expected_value(0, row, col), func.evaluate(inside_point, state, row, col));

      const auto jacobian = func.jacobian(inside_point, state, row, col);
      for (size_t ss = 0; ss < dim_state; ++ss)
        EXPECT_DOUBLE_EQ(expected_jacobian(0, row, col, ss), jacobian[ss]);
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
  func.evaluate(inside_point, state, values);
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1)
      EXPECT_DOUBLE_EQ(expected_value(0, row, 0), values[row]);
    else
      for (size_t col = 0; col < rC; ++col)
        EXPECT_DOUBLE_EQ(expected_value(0, row, col), values.get_entry(row, col));
  }

  typename FunctionType::DynamicJacobianRangeType jacobian;
  func.jacobian(inside_point, state, jacobian);
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1) {
      for (size_t ss = 0; ss < dim_state; ++ss)
        EXPECT_DOUBLE_EQ(expected_jacobian(0, row, 0, ss), jacobian.get_entry(row, ss));
    } else {
      for (size_t col = 0; col < rC; ++col)
        for (size_t ss = 0; ss < dim_state; ++ss)
          EXPECT_DOUBLE_EQ(expected_jacobian(0, row, col, ss), jacobian[row].get_entry(col, ss));
    }
  }
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
  EXPECT_DOUBLE_EQ(expected_value(0, 0, 0), local_function->evaluate(inside_point, state, 0, 0));
} // ... check_flux_function_defaults(...)


TEST_F(FluxFunctionInterfaceTest, flux_function_defaults)
{
  check_flux_function_defaults<2, 1>(element_);
  check_flux_function_defaults<2, 3>(element_);
}


} // namespace
