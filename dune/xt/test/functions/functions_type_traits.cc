// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-xt developers

#include <dune/xt/test/main.hxx> // <- Has to come first, includes the config.h!

#include <string>
#include <type_traits>

#include <dune/common/dynvector.hh>
#include <dune/common/fmatrix.hh>
#include <dune/common/fvector.hh>

#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/common/fvector.hh>
#include <dune/xt/la/container/common.hh>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/xt/functions/generic/function.hh>
#include <dune/xt/functions/generic/grid-function.hh>
#include <dune/xt/functions/interfaces/element-functions.hh>
#include <dune/xt/functions/interfaces/function.hh>
#include <dune/xt/functions/interfaces/grid-function.hh>
#include <dune/xt/functions/type_traits.hh>

using namespace Dune;
using namespace Dune::XT::Functions;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using E = XT::Grid::extract_entity_t<G>;
static constexpr size_t d = G::dimension;

// Aliases for everything used inside an EXPECT_* macro: the preprocessor does not know about angle
// brackets, so `Foo<E, 2, 3, double>` would be split into several macro arguments.
using ElementFunctionInterfaceType = ElementFunctionInterface<E, 2, 3, double>;
using FunctionInterfaceType = FunctionInterface<d, 2, 3, double>;
using GridFunctionInterfaceType = GridFunctionInterface<E, 2, 3, double>;
using GenericFunctionType = GenericFunction<d, 2, 3, double>;
using GenericGridFunctionType = GenericGridFunction<E, 2, 3, double>;


// RangeTypeSelector, general case (rC > 1)

GTEST_TEST(RangeTypeSelector, matrix_valued_types)
{
  using S = RangeTypeSelector<double, 2, 3>;
  EXPECT_TRUE((std::is_same_v<S::type, FieldMatrix<double, 2, 3>>));
  EXPECT_TRUE((std::is_same_v<S::return_type, XT::Common::FieldMatrix<double, 2, 3>>));
  EXPECT_TRUE((std::is_same_v<S::dynamic_type, XT::LA::CommonDenseMatrix<double>>));
}

GTEST_TEST(RangeTypeSelector, matrix_valued_ensure_size_grows_empty)
{
  using S = RangeTypeSelector<double, 2, 3>;
  S::dynamic_type mat;
  ASSERT_EQ(size_t(0), mat.rows());
  ASSERT_EQ(size_t(0), mat.cols());
  S::ensure_size(mat);
  EXPECT_EQ(size_t(2), mat.rows());
  EXPECT_EQ(size_t(3), mat.cols());
}

GTEST_TEST(RangeTypeSelector, matrix_valued_ensure_size_grows_if_only_cols_too_small)
{
  // covers the second operand of the `rows() < r || cols() < rC` short circuit
  using S = RangeTypeSelector<double, 2, 3>;
  S::dynamic_type mat(4, 1, 0.);
  S::ensure_size(mat);
  EXPECT_EQ(size_t(2), mat.rows());
  EXPECT_EQ(size_t(3), mat.cols());
}

GTEST_TEST(RangeTypeSelector, matrix_valued_ensure_size_keeps_larger)
{
  using S = RangeTypeSelector<double, 2, 3>;
  S::dynamic_type mat(4, 5, 0.);
  S::ensure_size(mat);
  EXPECT_EQ(size_t(4), mat.rows());
  EXPECT_EQ(size_t(5), mat.cols());
}

GTEST_TEST(RangeTypeSelector, matrix_valued_convert)
{
  static constexpr size_t r = 2;
  static constexpr size_t rC = 3;
  using S = RangeTypeSelector<double, r, rC>;
  S::type in;
  for (size_t ii = 0; ii < r; ++ii)
    for (size_t jj = 0; jj < rC; ++jj)
      in[ii][jj] = double(ii * rC + jj);
  S::dynamic_type out;
  S::ensure_size(out);
  S::convert(in, out);
  for (size_t ii = 0; ii < r; ++ii)
    for (size_t jj = 0; jj < rC; ++jj)
      EXPECT_DOUBLE_EQ(double(ii * rC + jj), out.get_entry(ii, jj));
}

GTEST_TEST(RangeTypeSelector, matrix_valued_convert_into_larger_target)
{
  static constexpr size_t r = 2;
  static constexpr size_t rC = 3;
  using S = RangeTypeSelector<double, r, rC>;
  S::type in;
  for (size_t ii = 0; ii < r; ++ii)
    for (size_t jj = 0; jj < rC; ++jj)
      in[ii][jj] = double(ii * rC + jj) + 1.;
  // convert only fills the upper left block and leaves the rest alone
  S::dynamic_type out(r + 1, rC + 1, 0.);
  S::convert(in, out);
  for (size_t ii = 0; ii < r; ++ii)
    for (size_t jj = 0; jj < rC; ++jj)
      EXPECT_DOUBLE_EQ(double(ii * rC + jj) + 1., out.get_entry(ii, jj));
  EXPECT_DOUBLE_EQ(0., out.get_entry(r, rC));
}


// RangeTypeSelector, rC == 1 specialization

GTEST_TEST(RangeTypeSelector, vector_valued_types)
{
  using S = RangeTypeSelector<double, 3, 1>;
  EXPECT_TRUE((std::is_same_v<S::type, FieldVector<double, 3>>));
  EXPECT_TRUE((std::is_same_v<S::return_type, XT::Common::FieldVector<double, 3>>));
  EXPECT_TRUE((std::is_same_v<S::dynamic_type, DynamicVector<double>>));
}

GTEST_TEST(RangeTypeSelector, vector_valued_ensure_size_grows)
{
  using S = RangeTypeSelector<double, 3, 1>;
  S::dynamic_type vec;
  ASSERT_EQ(size_t(0), vec.size());
  S::ensure_size(vec);
  EXPECT_EQ(size_t(3), vec.size());
}

GTEST_TEST(RangeTypeSelector, vector_valued_ensure_size_keeps_larger)
{
  using S = RangeTypeSelector<double, 3, 1>;
  S::dynamic_type vec;
  vec.resize(5);
  S::ensure_size(vec);
  EXPECT_EQ(size_t(5), vec.size());
}

GTEST_TEST(RangeTypeSelector, vector_valued_convert)
{
  static constexpr size_t r = 3;
  using S = RangeTypeSelector<double, r, 1>;
  S::type in;
  for (size_t ii = 0; ii < r; ++ii)
    in[ii] = double(ii) + 1.;
  S::dynamic_type out;
  S::ensure_size(out);
  S::convert(in, out);
  for (size_t ii = 0; ii < r; ++ii)
    EXPECT_DOUBLE_EQ(double(ii) + 1., out[ii]);
  // converting into a larger target leaves the tail alone
  S::dynamic_type larger_out;
  larger_out.resize(r + 2);
  S::convert(in, larger_out);
  for (size_t ii = 0; ii < r; ++ii)
    EXPECT_DOUBLE_EQ(double(ii) + 1., larger_out[ii]);
  EXPECT_DOUBLE_EQ(0., larger_out[r]);
  EXPECT_DOUBLE_EQ(0., larger_out[r + 1]);
}


// DerivativeRangeTypeSelector, general case (rC > 1)

GTEST_TEST(DerivativeRangeTypeSelector, matrix_valued_types)
{
  using S = DerivativeRangeTypeSelector<d, double, 2, 3>;
  EXPECT_TRUE((std::is_same_v<S::single_type, FieldVector<double, d>>));
  EXPECT_TRUE((std::is_same_v<S::return_single_type, XT::Common::FieldVector<double, d>>));
  EXPECT_TRUE((std::is_same_v<S::dynamic_single_type, DynamicVector<double>>));
  EXPECT_TRUE((std::is_same_v<S::row_derivative_type, FieldMatrix<double, 3, d>>));
  EXPECT_TRUE((std::is_same_v<S::row_derivative_return_type, XT::Common::FieldMatrix<double, 3, d>>));
  EXPECT_TRUE((std::is_same_v<S::dynamic_row_derivative_type, XT::LA::CommonDenseMatrix<double>>));
  EXPECT_TRUE((std::is_same_v<S::type, FieldVector<FieldMatrix<double, 3, d>, 2>>));
}

GTEST_TEST(DerivativeRangeTypeSelector, matrix_valued_ensure_size_grows_empty)
{
  static constexpr size_t r = 2;
  static constexpr size_t rC = 3;
  using S = DerivativeRangeTypeSelector<d, double, r, rC>;
  S::dynamic_type jac;
  ASSERT_EQ(size_t(0), jac.size());
  S::ensure_size(jac);
  ASSERT_EQ(r, jac.size());
  for (size_t ii = 0; ii < r; ++ii) {
    EXPECT_EQ(rC, jac[ii].rows());
    EXPECT_EQ(d, jac[ii].cols());
  }
}

GTEST_TEST(DerivativeRangeTypeSelector, matrix_valued_ensure_size_grows_if_only_cols_too_small)
{
  // the outer vector is large enough and the matrices have enough rows, so this covers the second
  // operand of the `arg[ii].rows() < rC || arg[ii].cols() < d` short circuit
  static constexpr size_t r = 2;
  static constexpr size_t rC = 3;
  using S = DerivativeRangeTypeSelector<d, double, r, rC>;
  S::dynamic_type jac;
  jac.resize(r);
  for (size_t ii = 0; ii < r; ++ii)
    jac[ii].resize(rC + 1, 1);
  S::ensure_size(jac);
  ASSERT_EQ(r, jac.size());
  for (size_t ii = 0; ii < r; ++ii) {
    EXPECT_EQ(rC, jac[ii].rows());
    EXPECT_EQ(d, jac[ii].cols());
  }
}

GTEST_TEST(DerivativeRangeTypeSelector, matrix_valued_ensure_size_keeps_larger)
{
  static constexpr size_t r = 2;
  static constexpr size_t rC = 3;
  using S = DerivativeRangeTypeSelector<d, double, r, rC>;
  S::dynamic_type jac;
  jac.resize(r + 1);
  for (size_t ii = 0; ii < jac.size(); ++ii)
    jac[ii].resize(rC + 1, d + 1);
  S::ensure_size(jac);
  ASSERT_EQ(r + 1, jac.size());
  for (size_t ii = 0; ii < jac.size(); ++ii) {
    EXPECT_EQ(rC + 1, jac[ii].rows());
    EXPECT_EQ(d + 1, jac[ii].cols());
  }
}

GTEST_TEST(DerivativeRangeTypeSelector, matrix_valued_convert)
{
  static constexpr size_t r = 2;
  static constexpr size_t rC = 3;
  using S = DerivativeRangeTypeSelector<d, double, r, rC>;
  S::type in;
  for (size_t ii = 0; ii < r; ++ii)
    for (size_t jj = 0; jj < rC; ++jj)
      for (size_t kk = 0; kk < d; ++kk)
        in[ii][jj][kk] = double((ii * rC + jj) * d + kk);
  S::dynamic_type out;
  S::ensure_size(out);
  S::convert(in, out);
  ASSERT_EQ(r, out.size());
  for (size_t ii = 0; ii < r; ++ii)
    for (size_t jj = 0; jj < rC; ++jj)
      for (size_t kk = 0; kk < d; ++kk)
        EXPECT_DOUBLE_EQ(double((ii * rC + jj) * d + kk), out[ii].get_entry(jj, kk));
}


// DerivativeRangeTypeSelector, rC == 1 specialization

GTEST_TEST(DerivativeRangeTypeSelector, vector_valued_types)
{
  using S = DerivativeRangeTypeSelector<d, double, 3, 1>;
  EXPECT_TRUE((std::is_same_v<S::single_type, FieldVector<double, d>>));
  EXPECT_TRUE((std::is_same_v<S::row_derivative_type, FieldMatrix<double, 3, d>>));
  EXPECT_TRUE((std::is_same_v<S::type, FieldMatrix<double, 3, d>>));
  EXPECT_TRUE((std::is_same_v<S::return_type, XT::Common::FieldMatrix<double, 3, d>>));
  EXPECT_TRUE((std::is_same_v<S::dynamic_type, XT::LA::CommonDenseMatrix<double>>));
  EXPECT_TRUE((std::is_same_v<S::dynamic_type, S::dynamic_row_derivative_type>));
}

GTEST_TEST(DerivativeRangeTypeSelector, vector_valued_ensure_size_grows_empty)
{
  using S = DerivativeRangeTypeSelector<d, double, 3, 1>;
  S::dynamic_type jac;
  ASSERT_EQ(size_t(0), jac.rows());
  S::ensure_size(jac);
  EXPECT_EQ(size_t(3), jac.rows());
  EXPECT_EQ(d, jac.cols());
}

GTEST_TEST(DerivativeRangeTypeSelector, vector_valued_ensure_size_grows_if_only_cols_too_small)
{
  // covers the second operand of the `rows() < r || cols() < d` short circuit
  using S = DerivativeRangeTypeSelector<d, double, 3, 1>;
  S::dynamic_type jac(4, 1, 0.);
  S::ensure_size(jac);
  EXPECT_EQ(size_t(3), jac.rows());
  EXPECT_EQ(d, jac.cols());
}

GTEST_TEST(DerivativeRangeTypeSelector, vector_valued_ensure_size_keeps_larger)
{
  using S = DerivativeRangeTypeSelector<d, double, 3, 1>;
  S::dynamic_type jac(4, d + 1, 0.);
  S::ensure_size(jac);
  EXPECT_EQ(size_t(4), jac.rows());
  EXPECT_EQ(d + 1, jac.cols());
}

GTEST_TEST(DerivativeRangeTypeSelector, vector_valued_convert)
{
  static constexpr size_t r = 3;
  using S = DerivativeRangeTypeSelector<d, double, r, 1>;
  S::type in;
  for (size_t ii = 0; ii < r; ++ii)
    for (size_t jj = 0; jj < d; ++jj)
      in[ii][jj] = double(ii * d + jj);
  S::dynamic_type out;
  S::ensure_size(out);
  S::convert(in, out);
  for (size_t ii = 0; ii < r; ++ii)
    for (size_t jj = 0; jj < d; ++jj)
      EXPECT_DOUBLE_EQ(double(ii * d + jj), out.get_entry(ii, jj));
}


// is_element_function / is_function / is_grid_function

GTEST_TEST(functions_type_traits, is_element_function)
{
  EXPECT_TRUE(is_element_function<ElementFunctionInterfaceType>::value);
  // not a candidate at all (no E, R, r, rC)
  EXPECT_FALSE(is_element_function<double>::value);
  EXPECT_FALSE(is_element_function<int>::value);
  EXPECT_FALSE(is_element_function<FunctionInterfaceType>::value);
  EXPECT_FALSE(is_element_function<GenericFunctionType>::value);
  // a candidate (provides E, R, r and rC), but not derived from a matching ElementFunctionInterface
  EXPECT_FALSE(is_element_function<GridFunctionInterfaceType>::value);
  EXPECT_FALSE(is_element_function<GenericGridFunctionType>::value);
}

GTEST_TEST(functions_type_traits, is_function)
{
  EXPECT_TRUE(is_function<FunctionInterfaceType>::value);
  EXPECT_TRUE(is_function<GenericFunctionType>::value);
  // not a candidate at all (no R, d, r, rC)
  EXPECT_FALSE(is_function<double>::value);
  EXPECT_FALSE(is_function<int>::value);
  // a candidate (provides R, d, r and rC), but not derived from a matching FunctionInterface
  EXPECT_FALSE(is_function<GridFunctionInterfaceType>::value);
  EXPECT_FALSE(is_function<ElementFunctionInterfaceType>::value);
}

GTEST_TEST(functions_type_traits, is_grid_function)
{
  EXPECT_TRUE(is_grid_function<GridFunctionInterfaceType>::value);
  EXPECT_TRUE(is_grid_function<GenericGridFunctionType>::value);
  // not a candidate at all (no E, R, r, rC)
  EXPECT_FALSE(is_grid_function<double>::value);
  EXPECT_FALSE(is_grid_function<int>::value);
  EXPECT_FALSE(is_grid_function<FunctionInterfaceType>::value);
  // a candidate (provides E, R, r and rC), but not derived from a matching GridFunctionInterface
  EXPECT_FALSE(is_grid_function<ElementFunctionInterfaceType>::value);
}


// as_element_function_interface / as_function_interface / as_grid_function_interface

GTEST_TEST(functions_type_traits, as_element_function_interface)
{
  EXPECT_TRUE(
      (std::is_same_v<as_element_function_interface_t<ElementFunctionInterfaceType>, ElementFunctionInterfaceType>));
  EXPECT_TRUE((
      std::is_same_v<as_element_function_interface<ElementFunctionInterfaceType>::type, ElementFunctionInterfaceType>));
  // the local function of a grid function is an element function of matching dimensions
  EXPECT_TRUE((std::is_same_v<as_element_function_interface_t<GridFunctionInterfaceType::LocalFunctionType>,
                              ElementFunctionInterfaceType>));
}

GTEST_TEST(functions_type_traits, as_function_interface)
{
  EXPECT_TRUE((std::is_same_v<as_function_interface_t<GenericFunctionType>, FunctionInterfaceType>));
  EXPECT_TRUE((std::is_same_v<as_function_interface_t<FunctionInterfaceType>, FunctionInterfaceType>));
  EXPECT_TRUE((std::is_same_v<as_function_interface<FunctionInterfaceType>::type, FunctionInterfaceType>));
}

GTEST_TEST(functions_type_traits, as_grid_function_interface)
{
  EXPECT_TRUE((std::is_same_v<as_grid_function_interface_t<GenericGridFunctionType>, GridFunctionInterfaceType>));
  EXPECT_TRUE((std::is_same_v<as_grid_function_interface_t<GridFunctionInterfaceType>, GridFunctionInterfaceType>));
  EXPECT_TRUE((std::is_same_v<as_grid_function_interface<GridFunctionInterfaceType>::type, GridFunctionInterfaceType>));
}


// CombinationType and GetCombination

GTEST_TEST(functions_type_traits, get_combination_name)
{
  EXPECT_EQ(std::string("difference"), get_combination_name(CombinationType::difference()));
  EXPECT_EQ(std::string("fraction"), get_combination_name(CombinationType::fraction()));
  EXPECT_EQ(std::string("product"), get_combination_name(CombinationType::product()));
  EXPECT_EQ(std::string("sum"), get_combination_name(CombinationType::sum()));
}

GTEST_TEST(functions_type_traits, get_combination_symbol)
{
  EXPECT_EQ(std::string("-"), get_combination_symbol(CombinationType::difference()));
  EXPECT_EQ(std::string("/"), get_combination_symbol(CombinationType::fraction()));
  EXPECT_EQ(std::string("*"), get_combination_symbol(CombinationType::product()));
  EXPECT_EQ(std::string("+"), get_combination_symbol(CombinationType::sum()));
}

GTEST_TEST(functions_type_traits, GetCombination_name)
{
  EXPECT_EQ(std::string("difference"), GetCombination<CombinationType::difference>::name());
  EXPECT_EQ(std::string("fraction"), GetCombination<CombinationType::fraction>::name());
  EXPECT_EQ(std::string("product"), GetCombination<CombinationType::product>::name());
  EXPECT_EQ(std::string("sum"), GetCombination<CombinationType::sum>::name());
}

GTEST_TEST(functions_type_traits, GetCombination_symbol)
{
  EXPECT_EQ(std::string("-"), GetCombination<CombinationType::difference>::symbol());
  EXPECT_EQ(std::string("/"), GetCombination<CombinationType::fraction>::symbol());
  EXPECT_EQ(std::string("*"), GetCombination<CombinationType::product>::symbol());
  EXPECT_EQ(std::string("+"), GetCombination<CombinationType::sum>::symbol());
}


// DerivativeType

GTEST_TEST(functions_type_traits, derivative_type)
{
  EXPECT_NE(int(DerivativeType::divergence), int(DerivativeType::gradient));
  auto which = DerivativeType::gradient;
  EXPECT_EQ(int(DerivativeType::gradient), int(which));
  which = DerivativeType::divergence;
  EXPECT_EQ(int(DerivativeType::divergence), int(which));
}
