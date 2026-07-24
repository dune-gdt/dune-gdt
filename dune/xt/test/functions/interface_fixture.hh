// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-xt developers

/// \file
/// \brief Shared scaffolding for the tests of the function and flux function interfaces.
///
/// element_function_interface.cc and element_flux_function_interface.cc both need a grid, one element to bind to and
/// the same deterministic expectation values; keeping that here rather than in both files avoids duplicating it.

#ifndef DUNE_XT_TEST_FUNCTIONS_INTERFACE_FIXTURE_HH
#define DUNE_XT_TEST_FUNCTIONS_INTERFACE_FIXTURE_HH

#include <array>
#include <cstddef>

#include <dune/common/fvector.hh>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/type_traits.hh>

#include <gtest/gtest.h>

namespace Dune::XT::Test {


/// \brief Provides a cube grid and one element of it, bindable by any of the element (flux) function interfaces.
struct FunctionInterfaceTestBase : public ::testing::Test
{
  using GridType = CUBEGRID_2D;
  using GridProviderType = Grid::GridProvider<GridType>;
  using ElementType = Grid::extract_entity_t<GridType>;
  static constexpr size_t dim_domain = GridType::dimension;
  using DomainType = FieldVector<double, dim_domain>;

  FunctionInterfaceTestBase()
    : grid_(Grid::make_cube_grid<GridType>(0., 1., 2))
    , leaf_view_(grid_.leaf_view())
    // which element we bind to is irrelevant: everything under test is element independent, it merely requires the
    // object to be bound at all
    , element_(*elements(leaf_view_).begin())
  {
  }

  const GridProviderType grid_;
  const typename GridProviderType::LeafGridViewType leaf_view_;
  const ElementType element_;
}; // struct FunctionInterfaceTestBase


/// \brief A point inside the reference element of a cube, which is [0, 1]^d.
inline FunctionInterfaceTestBase::DomainType inside_point()
{
  return FunctionInterfaceTestBase::DomainType(0.25);
}

/// \brief A point outside that reference element, to trip assert_inside_reference_element().
inline FunctionInterfaceTestBase::DomainType outside_point()
{
  return FunctionInterfaceTestBase::DomainType(1.5);
}

/// \brief Deterministic, pairwise distinct values, so every accessed component can be identified in an assertion.
inline double expected_value(const size_t function_index, const size_t row, const size_t col)
{
  return 100. * function_index + 10. * row + col + 1.;
}

/// \brief As expected_value(), for a derivative/jacobian component in direction dd.
inline double expected_derivative(const size_t function_index, const size_t row, const size_t col, const size_t dd)
{
  return 1000. * function_index + 100. * row + 10. * col + dd + 1.;
}

/// \brief The multi-index (value, ..., value), used to pick the derivative order.
inline std::array<size_t, FunctionInterfaceTestBase::dim_domain> multiindex(const size_t value)
{
  std::array<size_t, FunctionInterfaceTestBase::dim_domain> alpha;
  alpha.fill(value);
  return alpha;
}


/**
 * \name Filling and checking the range/derivative containers.
 *
 * A range is indexed [row] for rC == 1 and [row][col] otherwise; a derivative appends the direction, [row][dd] resp.
 * [row][col][dd]. Both shapes are shared between the plain function and the flux function interfaces -- the latter
 * only differ in that the trailing dimension of a derivative is the state dimension rather than the domain dimension,
 * which is what the `dim` parameter stands for. The dynamic_* variants take the same shapes in the dynamic containers,
 * where the innermost two indices become a get_entry(...) call.
 * \{
 */

template <size_t r, size_t rC, class RangeType>
RangeType filled_range(const size_t function_index)
{
  RangeType result;
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1)
      result[row] = expected_value(function_index, row, 0);
    else
      for (size_t col = 0; col < rC; ++col)
        result[row][col] = expected_value(function_index, row, col);
  }
  return result;
} // ... filled_range(...)

template <size_t r, size_t rC, size_t dim, class DerivativeRangeType>
DerivativeRangeType filled_derivative(const size_t function_index)
{
  DerivativeRangeType result;
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1) {
      for (size_t dd = 0; dd < dim; ++dd)
        result[row][dd] = expected_derivative(function_index, row, 0, dd);
    } else {
      for (size_t col = 0; col < rC; ++col)
        for (size_t dd = 0; dd < dim; ++dd)
          result[row][col][dd] = expected_derivative(function_index, row, col, dd);
    }
  }
  return result;
} // ... filled_derivative(...)

template <size_t r, size_t rC, class RangeType>
void expect_range_eq(const RangeType& actual, const size_t function_index)
{
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1)
      EXPECT_DOUBLE_EQ(expected_value(function_index, row, 0), actual[row]);
    else
      for (size_t col = 0; col < rC; ++col)
        EXPECT_DOUBLE_EQ(expected_value(function_index, row, col), actual[row][col]);
  }
} // ... expect_range_eq(...)

template <size_t r, size_t rC, class DynamicRangeType>
void expect_dynamic_range_eq(const DynamicRangeType& actual, const size_t function_index)
{
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1)
      EXPECT_DOUBLE_EQ(expected_value(function_index, row, 0), actual[row]);
    else
      for (size_t col = 0; col < rC; ++col)
        EXPECT_DOUBLE_EQ(expected_value(function_index, row, col), actual.get_entry(row, col));
  }
} // ... expect_dynamic_range_eq(...)

template <size_t r, size_t rC, size_t dim, class DerivativeRangeType>
void expect_derivative_eq(const DerivativeRangeType& actual, const size_t function_index)
{
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1) {
      for (size_t dd = 0; dd < dim; ++dd)
        EXPECT_DOUBLE_EQ(expected_derivative(function_index, row, 0, dd), actual[row][dd]);
    } else {
      for (size_t col = 0; col < rC; ++col)
        for (size_t dd = 0; dd < dim; ++dd)
          EXPECT_DOUBLE_EQ(expected_derivative(function_index, row, col, dd), actual[row][col][dd]);
    }
  }
} // ... expect_derivative_eq(...)

template <size_t r, size_t rC, size_t dim, class DynamicDerivativeRangeType>
void expect_dynamic_derivative_eq(const DynamicDerivativeRangeType& actual, const size_t function_index)
{
  for (size_t row = 0; row < r; ++row) {
    if constexpr (rC == 1) {
      for (size_t dd = 0; dd < dim; ++dd)
        EXPECT_DOUBLE_EQ(expected_derivative(function_index, row, 0, dd), actual.get_entry(row, dd));
    } else {
      for (size_t col = 0; col < rC; ++col)
        for (size_t dd = 0; dd < dim; ++dd)
          EXPECT_DOUBLE_EQ(expected_derivative(function_index, row, col, dd), actual[row].get_entry(col, dd));
    }
  }
} // ... expect_dynamic_derivative_eq(...)

/// \}


} // namespace Dune::XT::Test

#endif // DUNE_XT_TEST_FUNCTIONS_INTERFACE_FIXTURE_HH
