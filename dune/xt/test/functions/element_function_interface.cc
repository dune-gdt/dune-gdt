// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-xt developers

/// \file
/// \brief Tests the default implementations provided by ElementFunctionSetInterface and ElementFunctionInterface.
///
/// The concrete implementations in dune/xt/functions override the "should be implemented" virtuals, which means the
/// convenience layers the interfaces provide on top of them (the single-component accessors, the dynamic-container
/// overloads, the argument checks and the NotImplemented defaults) are never exercised by a test of a concrete
/// function. The minimal subclasses below override exactly as much as each test needs and leave the rest to the
/// interface, so those layers are reached.

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- has to come first, includes the config.h!

#include <array>
#include <vector>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/xt/functions/exceptions.hh>
#include <dune/xt/functions/interfaces/element-functions.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::Functions;

namespace {


using GridType = CUBEGRID_2D;
using ElementType = XT::Grid::extract_entity_t<GridType>;
constexpr size_t dim_domain = GridType::dimension;

// the reference element of a YaspGrid cube is [0, 1]^d
const FieldVector<double, dim_domain> inside_point(0.25);
const FieldVector<double, dim_domain> outside_point(1.5);


/// \brief Deterministic, pairwise distinct values, so every accessed component can be identified in an assertion.
double expected_value(const size_t function_index, const size_t row, const size_t col)
{
  return 100. * function_index + 10. * row + col + 1.;
}

double expected_derivative(const size_t function_index, const size_t row, const size_t col, const size_t dd)
{
  return 1000. * function_index + 100. * row + 10. * col + dd + 1.;
}

std::array<size_t, dim_domain> first_derivative_multiindex()
{
  std::array<size_t, dim_domain> alpha;
  alpha.fill(1);
  return alpha;
}

std::array<size_t, dim_domain> higher_derivative_multiindex()
{
  std::array<size_t, dim_domain> alpha;
  alpha.fill(2);
  return alpha;
}


/// \brief Implements only the three pure virtuals, so every "should be implemented" method still throws.
template <size_t r, size_t rC>
class ThrowingSet : public ElementFunctionSetInterface<ElementType, r, rC>
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


/// \brief Implements evaluate() and jacobians() only; everything else is served by the interface.
template <size_t r, size_t rC>
class FixedSet : public ElementFunctionSetInterface<ElementType, r, rC>
{
  using BaseType = ElementFunctionSetInterface<ElementType, r, rC>;

public:
  using BaseType::d;
  using typename BaseType::DerivativeRangeType;
  using typename BaseType::DomainType;
  using typename BaseType::RangeType;

  // overriding one overload hides the others, and those are exactly what is under test here
  using BaseType::evaluate;
  using BaseType::jacobians;

  static constexpr size_t set_size = 2;

  size_t size(const Common::Parameter& /*param*/ = {}) const override
  {
    return set_size;
  }

  size_t max_size(const Common::Parameter& /*param*/ = {}) const override
  {
    return set_size;
  }

  int order(const Common::Parameter& /*param*/ = {}) const override
  {
    return 0;
  }

  void evaluate(const DomainType& point_in_reference_element,
                std::vector<RangeType>& result,
                const Common::Parameter& /*param*/ = {}) const override
  {
    this->assert_inside_reference_element(point_in_reference_element);
    if (result.size() < set_size)
      result.resize(set_size);
    for (size_t ii = 0; ii < set_size; ++ii)
      for (size_t row = 0; row < r; ++row) {
        if constexpr (rC == 1)
          result[ii][row] = expected_value(ii, row, 0);
        else
          for (size_t col = 0; col < rC; ++col)
            result[ii][row][col] = expected_value(ii, row, col);
      }
  } // ... evaluate(...)

  void jacobians(const DomainType& point_in_reference_element,
                 std::vector<DerivativeRangeType>& result,
                 const Common::Parameter& /*param*/ = {}) const override
  {
    this->assert_inside_reference_element(point_in_reference_element);
    if (result.size() < set_size)
      result.resize(set_size);
    for (size_t ii = 0; ii < set_size; ++ii)
      for (size_t row = 0; row < r; ++row) {
        if constexpr (rC == 1) {
          for (size_t dd = 0; dd < d; ++dd)
            result[ii][row][dd] = expected_derivative(ii, row, 0, dd);
        } else {
          for (size_t col = 0; col < rC; ++col)
            for (size_t dd = 0; dd < d; ++dd)
              result[ii][row][col][dd] = expected_derivative(ii, row, col, dd);
        }
      }
  } // ... jacobians(...)
};


/// \brief Implements only order(), so evaluate()/jacobian()/derivative() still throw.
template <size_t r, size_t rC>
class ThrowingFunction : public ElementFunctionInterface<ElementType, r, rC>
{
public:
  int order(const Common::Parameter& /*param*/ = {}) const override
  {
    return 0;
  }
};


/// \brief Implements the three single-valued methods only; everything else is served by the interface.
template <size_t r, size_t rC>
class FixedFunction : public ElementFunctionInterface<ElementType, r, rC>
{
  using BaseType = ElementFunctionInterface<ElementType, r, rC>;

public:
  using BaseType::d;
  using typename BaseType::DerivativeRangeReturnType;
  using typename BaseType::DomainType;
  using typename BaseType::RangeReturnType;

  // overriding one overload hides the others, and those are exactly what is under test here
  using BaseType::derivative;
  using BaseType::evaluate;
  using BaseType::jacobian;

  /// \note jacobian() reports the values of function_index 0, derivative() those of 1, so the tests below can tell
  ///       which of the two an interface method dispatched to.
  static constexpr size_t jacobian_index = 0;
  static constexpr size_t derivative_index = 1;

  int order(const Common::Parameter& /*param*/ = {}) const override
  {
    return 0;
  }

  RangeReturnType evaluate(const DomainType& point_in_reference_element,
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

  DerivativeRangeReturnType jacobian(const DomainType& point_in_reference_element,
                                     const Common::Parameter& /*param*/ = {}) const override
  {
    this->assert_inside_reference_element(point_in_reference_element);
    return filled_derivative(jacobian_index);
  }

  DerivativeRangeReturnType derivative(const std::array<size_t, d>& /*alpha*/,
                                       const DomainType& point_in_reference_element,
                                       const Common::Parameter& /*param*/ = {}) const override
  {
    this->assert_inside_reference_element(point_in_reference_element);
    return filled_derivative(derivative_index);
  }

  static DerivativeRangeReturnType filled_derivative(const size_t function_index)
  {
    DerivativeRangeReturnType result;
    for (size_t row = 0; row < r; ++row) {
      if constexpr (rC == 1) {
        for (size_t dd = 0; dd < d; ++dd)
          result[row][dd] = expected_derivative(function_index, row, 0, dd);
      } else {
        for (size_t col = 0; col < rC; ++col)
          for (size_t dd = 0; dd < d; ++dd)
            result[row][col][dd] = expected_derivative(function_index, row, col, dd);
      }
    }
    return result;
  } // ... filled_derivative(...)
};


/// \brief Component-wise check of a (set of) DerivativeRangeType against expected_derivative().
template <size_t r, size_t rC, class DerivativeType>
void expect_derivative_eq(const DerivativeType& actual, const size_t function_index)
{
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1) {
      for (size_t dd = 0; dd < dim_domain; ++dd)
        EXPECT_DOUBLE_EQ(expected_derivative(function_index, row, 0, dd), actual[row][dd]);
    } else {
      for (size_t col = 0; col < rC; ++col)
        for (size_t dd = 0; dd < dim_domain; ++dd)
          EXPECT_DOUBLE_EQ(expected_derivative(function_index, row, col, dd), actual[row][col][dd]);
    }
  }
} // ... expect_derivative_eq(...)


struct ElementFunctionInterfaceTest : public ::testing::Test
{
  using GridProviderType = XT::Grid::GridProvider<GridType>;

  ElementFunctionInterfaceTest()
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
}; // struct ElementFunctionInterfaceTest


// ============================================================================ ElementFunctionSetInterface


template <size_t r, size_t rC>
void check_set_defaults_throw(const ElementType& element)
{
  ThrowingSet<r, rC> set;
  set.bind(element);

  std::vector<typename ThrowingSet<r, rC>::RangeType> values;
  std::vector<typename ThrowingSet<r, rC>::DerivativeRangeType> derivatives;

  EXPECT_THROW(set.evaluate(inside_point, values), Dune::NotImplemented);
  EXPECT_THROW(set.jacobians(inside_point, derivatives), Dune::NotImplemented);
  // the default derivatives() forwards the all-ones multi-index to jacobians(), which throws in turn ...
  EXPECT_THROW(set.derivatives(first_derivative_multiindex(), inside_point, derivatives), Dune::NotImplemented);
  // ... and rejects any other multi-index itself
  EXPECT_THROW(set.derivatives(higher_derivative_multiindex(), inside_point, derivatives), Dune::NotImplemented);
} // ... check_set_defaults_throw(...)


template <size_t r, size_t rC>
void check_set_of_set_wrappers(const ElementType& element)
{
  FixedSet<r, rC> set;
  set.bind(element);

  const auto values = set.evaluate_set(inside_point);
  ASSERT_EQ(FixedSet<r, rC>::set_size, values.size());
  for (size_t ii = 0; ii < values.size(); ++ii)
    for (size_t row = 0; row < r; ++row) {
      if constexpr (rC == 1)
        EXPECT_DOUBLE_EQ(expected_value(ii, row, 0), values[ii][row]);
      else
        for (size_t col = 0; col < rC; ++col)
          EXPECT_DOUBLE_EQ(expected_value(ii, row, col), values[ii][row][col]);
    }

  const auto jacobians = set.jacobians_of_set(inside_point);
  ASSERT_EQ(FixedSet<r, rC>::set_size, jacobians.size());
  for (size_t ii = 0; ii < jacobians.size(); ++ii)
    expect_derivative_eq<r, rC>(jacobians[ii], ii);

  // derivatives_of_set() sizes the result from size() and forwards to derivatives(), whose default forwards the
  // all-ones multi-index to jacobians() -- so this has to reproduce the jacobian values
  const auto derivatives = set.derivatives_of_set(first_derivative_multiindex(), inside_point);
  ASSERT_EQ(FixedSet<r, rC>::set_size, derivatives.size());
  for (size_t ii = 0; ii < derivatives.size(); ++ii)
    expect_derivative_eq<r, rC>(derivatives[ii], ii);
} // ... check_set_of_set_wrappers(...)


template <size_t r, size_t rC>
void check_set_single_component_accessors(const ElementType& element)
{
  FixedSet<r, rC> set;
  set.bind(element);
  using SingleDerivativeRangeType = typename FixedSet<r, rC>::SingleDerivativeRangeType;

  for (size_t row = 0; row < r; ++row)
    for (size_t col = 0; col < rC; ++col) {
      // all three results are deliberately left empty, to also cover the interface's resize path
      std::vector<double> values;
      set.evaluate(inside_point, values, row, col);
      ASSERT_EQ(FixedSet<r, rC>::set_size, values.size());
      for (size_t ii = 0; ii < values.size(); ++ii)
        EXPECT_DOUBLE_EQ(expected_value(ii, row, col), values[ii]);

      std::vector<SingleDerivativeRangeType> jacobians;
      set.jacobians(inside_point, jacobians, row, col);
      ASSERT_EQ(FixedSet<r, rC>::set_size, jacobians.size());
      for (size_t ii = 0; ii < jacobians.size(); ++ii)
        for (size_t dd = 0; dd < dim_domain; ++dd)
          EXPECT_DOUBLE_EQ(expected_derivative(ii, row, col, dd), jacobians[ii][dd]);

      std::vector<SingleDerivativeRangeType> derivatives;
      set.derivatives(first_derivative_multiindex(), inside_point, derivatives, row, col);
      ASSERT_EQ(FixedSet<r, rC>::set_size, derivatives.size());
      for (size_t ii = 0; ii < derivatives.size(); ++ii)
        for (size_t dd = 0; dd < dim_domain; ++dd)
          EXPECT_DOUBLE_EQ(expected_derivative(ii, row, col, dd), derivatives[ii][dd]);
    }
} // ... check_set_single_component_accessors(...)


template <size_t r, size_t rC>
void check_set_dynamic_overloads(const ElementType& element)
{
  FixedSet<r, rC> set;
  set.bind(element);
  using DynamicRangeType = typename FixedSet<r, rC>::DynamicRangeType;
  using DynamicDerivativeRangeType = typename FixedSet<r, rC>::DynamicDerivativeRangeType;

  // all results are deliberately left empty, so the interface has to resize and ensure_size the entries
  std::vector<DynamicRangeType> values;
  set.evaluate(inside_point, values);
  ASSERT_EQ(FixedSet<r, rC>::set_size, values.size());
  for (size_t ii = 0; ii < values.size(); ++ii)
    for (size_t row = 0; row < r; ++row) {
      if constexpr (rC == 1)
        EXPECT_DOUBLE_EQ(expected_value(ii, row, 0), values[ii][row]);
      else
        for (size_t col = 0; col < rC; ++col)
          EXPECT_DOUBLE_EQ(expected_value(ii, row, col), values[ii].get_entry(row, col));
    }

  std::vector<DynamicDerivativeRangeType> jacobians;
  set.jacobians(inside_point, jacobians);
  ASSERT_EQ(FixedSet<r, rC>::set_size, jacobians.size());

  std::vector<DynamicDerivativeRangeType> derivatives;
  set.derivatives(first_derivative_multiindex(), inside_point, derivatives);
  ASSERT_EQ(FixedSet<r, rC>::set_size, derivatives.size());

  for (size_t ii = 0; ii < jacobians.size(); ++ii)
    for (size_t row = 0; row < r; ++row) {
      if constexpr (rC == 1) {
        for (size_t dd = 0; dd < dim_domain; ++dd) {
          EXPECT_DOUBLE_EQ(expected_derivative(ii, row, 0, dd), jacobians[ii].get_entry(row, dd));
          EXPECT_DOUBLE_EQ(expected_derivative(ii, row, 0, dd), derivatives[ii].get_entry(row, dd));
        }
      } else {
        for (size_t col = 0; col < rC; ++col)
          for (size_t dd = 0; dd < dim_domain; ++dd) {
            EXPECT_DOUBLE_EQ(expected_derivative(ii, row, col, dd), jacobians[ii][row].get_entry(col, dd));
            EXPECT_DOUBLE_EQ(expected_derivative(ii, row, col, dd), derivatives[ii][row].get_entry(col, dd));
          }
      }
    }
} // ... check_set_dynamic_overloads(...)


template <size_t r, size_t rC>
void check_set_argument_checks(const ElementType& element)
{
  FixedSet<r, rC> set;
  set.bind(element);
  using SingleDerivativeRangeType = typename FixedSet<r, rC>::SingleDerivativeRangeType;

  std::vector<double> values;
  std::vector<SingleDerivativeRangeType> derivatives;

  EXPECT_THROW(set.evaluate(inside_point, values, /*row=*/r, /*col=*/0), Common::Exceptions::shapes_do_not_match);
  EXPECT_THROW(set.jacobians(inside_point, derivatives, /*row=*/0, /*col=*/rC),
               Common::Exceptions::shapes_do_not_match);
  EXPECT_THROW(set.derivatives(first_derivative_multiindex(), inside_point, derivatives, /*row=*/r, /*col=*/0),
               Common::Exceptions::shapes_do_not_match);

  // assert_inside_reference_element() is what FixedSet calls at the top of evaluate()/jacobians()
  std::vector<typename FixedSet<r, rC>::RangeType> range_values;
  EXPECT_THROW(set.evaluate(outside_point, range_values), Functions::Exceptions::wrong_input_given);
} // ... check_set_argument_checks(...)


TEST_F(ElementFunctionInterfaceTest, set_defaults_throw)
{
  check_set_defaults_throw<2, 1>(element_);
  check_set_defaults_throw<2, 3>(element_);
}

TEST_F(ElementFunctionInterfaceTest, set_of_set_wrappers)
{
  check_set_of_set_wrappers<2, 1>(element_);
  check_set_of_set_wrappers<2, 3>(element_);
}

TEST_F(ElementFunctionInterfaceTest, set_single_component_accessors)
{
  check_set_single_component_accessors<2, 1>(element_);
  check_set_single_component_accessors<2, 3>(element_);
}

TEST_F(ElementFunctionInterfaceTest, set_dynamic_overloads)
{
  check_set_dynamic_overloads<2, 1>(element_);
  check_set_dynamic_overloads<2, 3>(element_);
}

TEST_F(ElementFunctionInterfaceTest, set_argument_checks)
{
  check_set_argument_checks<2, 1>(element_);
  check_set_argument_checks<2, 3>(element_);
}


// =============================================================================== ElementFunctionInterface


template <size_t r, size_t rC>
void check_function_defaults_throw(const ElementType& element)
{
  ThrowingFunction<r, rC> func;
  func.bind(element);

  EXPECT_THROW(func.evaluate(inside_point), Dune::NotImplemented);
  EXPECT_THROW(func.jacobian(inside_point), Dune::NotImplemented);
  EXPECT_THROW(func.derivative(first_derivative_multiindex(), inside_point), Dune::NotImplemented);
} // ... check_function_defaults_throw(...)


template <size_t r, size_t rC>
void check_function_set_bridge(const ElementType& element)
{
  using FunctionType = FixedFunction<r, rC>;
  FunctionType func;
  func.bind(element);

  // a single function is a set of size one
  EXPECT_EQ(1u, func.size());
  EXPECT_EQ(1u, func.max_size());

  // the set-flavoured methods have to forward to the single-valued ones; all results are deliberately left empty
  std::vector<typename FunctionType::RangeType> values;
  func.evaluate(inside_point, values);
  ASSERT_EQ(1u, values.size());
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1)
      EXPECT_DOUBLE_EQ(expected_value(0, row, 0), values[0][row]);
    else
      for (size_t col = 0; col < rC; ++col)
        EXPECT_DOUBLE_EQ(expected_value(0, row, col), values[0][row][col]);
  }

  std::vector<typename FunctionType::DerivativeRangeType> jacobians;
  func.jacobians(inside_point, jacobians);
  ASSERT_EQ(1u, jacobians.size());
  expect_derivative_eq<r, rC>(jacobians[0], FunctionType::jacobian_index);

  std::vector<typename FunctionType::DerivativeRangeType> derivatives;
  func.derivatives(first_derivative_multiindex(), inside_point, derivatives);
  ASSERT_EQ(1u, derivatives.size());
  // derivative() reports different values than jacobian(), so this confirms which one was dispatched to
  expect_derivative_eq<r, rC>(derivatives[0], FunctionType::derivative_index);
} // ... check_function_set_bridge(...)


template <size_t r, size_t rC>
void check_function_single_component_accessors(const ElementType& element)
{
  using FunctionType = FixedFunction<r, rC>;
  FunctionType func;
  func.bind(element);

  for (size_t row = 0; row < r; ++row)
    for (size_t col = 0; col < rC; ++col) {
      EXPECT_DOUBLE_EQ(expected_value(0, row, col), func.evaluate(inside_point, row, col));

      const auto jacobian = func.jacobian(inside_point, row, col);
      for (size_t dd = 0; dd < dim_domain; ++dd)
        EXPECT_DOUBLE_EQ(expected_derivative(FunctionType::jacobian_index, row, col, dd), jacobian[dd]);

      const auto derivative = func.derivative(first_derivative_multiindex(), inside_point, row, col);
      for (size_t dd = 0; dd < dim_domain; ++dd)
        EXPECT_DOUBLE_EQ(expected_derivative(FunctionType::derivative_index, row, col, dd), derivative[dd]);
    }
} // ... check_function_single_component_accessors(...)


template <size_t r, size_t rC>
void check_function_dynamic_overloads(const ElementType& element)
{
  using FunctionType = FixedFunction<r, rC>;
  FunctionType func;
  func.bind(element);

  // all results are default constructed, so the interface has to ensure_size them
  typename FunctionType::DynamicRangeType values;
  func.evaluate(inside_point, values);
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1)
      EXPECT_DOUBLE_EQ(expected_value(0, row, 0), values[row]);
    else
      for (size_t col = 0; col < rC; ++col)
        EXPECT_DOUBLE_EQ(expected_value(0, row, col), values.get_entry(row, col));
  }

  typename FunctionType::DynamicDerivativeRangeType jacobian;
  func.jacobian(inside_point, jacobian);
  typename FunctionType::DynamicDerivativeRangeType derivative;
  func.derivative(first_derivative_multiindex(), inside_point, derivative);
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1) {
      for (size_t dd = 0; dd < dim_domain; ++dd) {
        EXPECT_DOUBLE_EQ(expected_derivative(FunctionType::jacobian_index, row, 0, dd), jacobian.get_entry(row, dd));
        EXPECT_DOUBLE_EQ(expected_derivative(FunctionType::derivative_index, row, 0, dd),
                         derivative.get_entry(row, dd));
      }
    } else {
      for (size_t col = 0; col < rC; ++col)
        for (size_t dd = 0; dd < dim_domain; ++dd) {
          EXPECT_DOUBLE_EQ(expected_derivative(FunctionType::jacobian_index, row, col, dd),
                           jacobian[row].get_entry(col, dd));
          EXPECT_DOUBLE_EQ(expected_derivative(FunctionType::derivative_index, row, col, dd),
                           derivative[row].get_entry(col, dd));
        }
    }
  }
} // ... check_function_dynamic_overloads(...)


template <size_t r, size_t rC>
void check_function_argument_checks(const ElementType& element)
{
  FixedFunction<r, rC> func;
  func.bind(element);

  EXPECT_THROW(func.evaluate(inside_point, /*row=*/r, /*col=*/0), Common::Exceptions::shapes_do_not_match);
  EXPECT_THROW(func.jacobian(inside_point, /*row=*/0, /*col=*/rC), Common::Exceptions::shapes_do_not_match);
  EXPECT_THROW(func.derivative(first_derivative_multiindex(), inside_point, /*row=*/r, /*col=*/0),
               Common::Exceptions::shapes_do_not_match);

  EXPECT_THROW(func.evaluate(outside_point), Functions::Exceptions::wrong_input_given);
} // ... check_function_argument_checks(...)


TEST_F(ElementFunctionInterfaceTest, function_defaults_throw)
{
  check_function_defaults_throw<2, 1>(element_);
  check_function_defaults_throw<2, 3>(element_);
}

TEST_F(ElementFunctionInterfaceTest, function_set_bridge)
{
  check_function_set_bridge<2, 1>(element_);
  check_function_set_bridge<2, 3>(element_);
}

TEST_F(ElementFunctionInterfaceTest, function_single_component_accessors)
{
  check_function_single_component_accessors<2, 1>(element_);
  check_function_single_component_accessors<2, 3>(element_);
}

TEST_F(ElementFunctionInterfaceTest, function_dynamic_overloads)
{
  check_function_dynamic_overloads<2, 1>(element_);
  check_function_dynamic_overloads<2, 3>(element_);
}

TEST_F(ElementFunctionInterfaceTest, function_argument_checks)
{
  check_function_argument_checks<2, 1>(element_);
  check_function_argument_checks<2, 3>(element_);
}


} // namespace
