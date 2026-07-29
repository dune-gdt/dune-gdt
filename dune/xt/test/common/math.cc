// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2012, 2014 - 2017)
//   René Fritze     (2012 - 2016, 2018 - 2019)
//   Tobias Leibner  (2014, 2016, 2019 - 2020)

#include <dune/xt/test/main.hxx>

#include <dune/common/dynmatrix.hh>
#include <dune/common/tupleutility.hh>

#include <limits>

#include <dune/xt/common/math.hh>
#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/ranges.hh>

using namespace Dune::XT::Common;
using namespace Dune::XT::Test;
using TestTypes = testing::Types<double, int>;
using ComplexTestTypes = testing::Types<std::complex<double>, double, int>;


template <class T>
struct ClampTest : public testing::Test
{
  const T lower;
  const T upper;
  ClampTest()
    : lower(-1)
    , upper(1)
  {
  }
};

TYPED_TEST_SUITE(ClampTest, TestTypes);
TYPED_TEST(ClampTest, All)
{
  using T = TypeParam;
  EXPECT_EQ(Dune::XT::Common::clamp(T(-2), this->lower, this->upper), this->lower);
  EXPECT_EQ(Dune::XT::Common::clamp(T(2), this->lower, this->upper), this->upper);
  EXPECT_EQ(Dune::XT::Common::clamp(T(0), this->lower, this->upper), T(0));
}

template <class T>
struct EpsilonTest : public testing::Test
{};

TYPED_TEST_SUITE(EpsilonTest, TestTypes);
TYPED_TEST(EpsilonTest, All)
{
  EXPECT_NE(Epsilon<TypeParam>::value, TypeParam(0));
}

template <class T>
struct MinMaxAvgTest : public testing::Test
{};

template <class MMType, class TypeParam>
void mmCheck(const MMType& mma)
{
  EXPECT_TRUE(Dune::FloatCmp::eq(mma.min(), TypeParam(-4.0)));
  EXPECT_TRUE(Dune::FloatCmp::eq(mma.max(), TypeParam(1.0)));
  EXPECT_TRUE(Dune::FloatCmp::eq(mma.average(), TypeParam(-1.0)));
}

TYPED_TEST_SUITE(MinMaxAvgTest, TestTypes);
TYPED_TEST(MinMaxAvgTest, All)
{
  MinMaxAvg<TypeParam> mma;
  mma(-1);
  mma(1);
  EXPECT_TRUE(Dune::FloatCmp::eq(mma.min(), TypeParam(-1.0)));
  EXPECT_TRUE(Dune::FloatCmp::eq(mma.max(), TypeParam(1.0)));
  EXPECT_TRUE(Dune::FloatCmp::eq(mma.average(), TypeParam(0.0)));
  mma(0);
  EXPECT_TRUE(Dune::FloatCmp::eq(mma.min(), TypeParam(-1.0)));
  EXPECT_TRUE(Dune::FloatCmp::eq(mma.max(), TypeParam(1.0)));
  EXPECT_TRUE(Dune::FloatCmp::eq(mma.average(), TypeParam(0.0)));
  mma(-4);
  mmCheck<MinMaxAvg<TypeParam>, TypeParam>(mma);
  auto mmb = mma;
  mmCheck<MinMaxAvg<TypeParam>, TypeParam>(mmb);
}

GTEST_TEST(OtherMath, Range)
{
  EXPECT_EQ((std::vector<unsigned int>{0, 1, 2, 3}), value_range(4u));
  EXPECT_EQ((std::vector<int>{4, 3, 2, 1}), value_range(4, 0, -1));
  EXPECT_EQ((std::vector<int>{-1, 0, 1}), value_range(-1, 2));
  EXPECT_EQ((std::vector<float>()), value_range(0.f));
  EXPECT_EQ((std::vector<float>{0.f}), value_range(Epsilon<float>::value));
  Dune::FieldMatrix<double, 2, 2> fMatrix;
  fMatrix = 0.0;
  EXPECT_DOUBLE_EQ(fMatrix[1][1], 0.0);
  Dune::DynamicMatrix<double> dMatrix(2, 2);
  dMatrix = 0.0;
  EXPECT_DOUBLE_EQ(dMatrix[1][1], 0.0);
}

GTEST_TEST(OtherMath, Sign)
{
  EXPECT_EQ(signum(1), 1);
  EXPECT_EQ(signum(-1), -1);
  EXPECT_EQ(signum(1.), 1);
  EXPECT_EQ(signum(-1.), -1);
}

GTEST_TEST(OtherMath, AbsoluteValue)
{
  EXPECT_EQ(abs(1.0f), 1.0f);
  EXPECT_EQ(abs(-1l), 1l);
  EXPECT_EQ(Dune::XT::Common::abs(0u), 0u);
  EXPECT_EQ(abs(0), 0);
  EXPECT_EQ(abs(std::complex<double>(0)), 0);
  EXPECT_EQ(abs(std::complex<double>(-1)), 1);
}

GTEST_TEST(OtherMath, MinAndMax)
{
  EXPECT_EQ(max(1., -1.), 1.);
  EXPECT_EQ(min(1., -1.), -1.);
  EXPECT_EQ(minmod(2., -1.), 0.);
  EXPECT_EQ(maxmod(2., -1.), 0.);
  EXPECT_EQ(minmod(2., 1.), 1.);
  EXPECT_EQ(maxmod(2., 1.), 2.);
  EXPECT_EQ(minmod(-2., -1.), -1.);
  EXPECT_EQ(maxmod(-2., -1.), -2.);
}

GTEST_TEST(OtherMath, BinomialCoefficient)
{
  // The classical cases: n over 0 is 1, n over 1 is n, and n over k is symmetric in k <-> n - k.
  EXPECT_DOUBLE_EQ(1., binomial_coefficient(5., 0));
  EXPECT_DOUBLE_EQ(5., binomial_coefficient(5., 1));
  EXPECT_DOUBLE_EQ(10., binomial_coefficient(5., 2));
  EXPECT_DOUBLE_EQ(10., binomial_coefficient(5., 3));
  EXPECT_DOUBLE_EQ(1., binomial_coefficient(5., 5));
  // k > n vanishes, since the factor (n + 1 - k) becomes zero for k == n + 1.
  EXPECT_DOUBLE_EQ(0., binomial_coefficient(5., 6));
  EXPECT_DOUBLE_EQ(0., binomial_coefficient(5., 7));
  // "for arbitrary n": the generalized coefficient is defined for non-integral (and negative) n as well.
  EXPECT_DOUBLE_EQ(1., binomial_coefficient(0.5, 0));
  EXPECT_DOUBLE_EQ(0.5, binomial_coefficient(0.5, 1));
  EXPECT_DOUBLE_EQ(0.5 * (-0.5) / 2., binomial_coefficient(0.5, 2));
  EXPECT_DOUBLE_EQ(-1., binomial_coefficient(-1., 1));
  EXPECT_DOUBLE_EQ(1., binomial_coefficient(-1., 2));
}

GTEST_TEST(OtherMath, AbsoluteValueOfChars)
{
  // There is no std::abs(char), so dune-xt provides its own in order to avoid the narrowing conversion.
  EXPECT_EQ(char(0), Dune::XT::Common::abs(char(0)));
  EXPECT_EQ(char(1), Dune::XT::Common::abs(char(1)));
  EXPECT_EQ(char(127), Dune::XT::Common::abs(char(127)));
  EXPECT_EQ(char(3), Dune::XT::Common::internal::abs(char(3)));
  // Whether plain char is signed is implementation defined (it is unsigned on e.g. arm and power). Where it is not,
  // char(-1) is simply the value 255 and abs() has nothing to do -- so only assert on negative inputs where there
  // are any.
  if constexpr (std::numeric_limits<char>::is_signed) {
    EXPECT_EQ(char(1), Dune::XT::Common::abs(char(-1)));
    EXPECT_EQ(char(3), Dune::XT::Common::internal::abs(char(-3)));
  }
}

GTEST_TEST(OtherMath, EpsilonOfStrings)
{
  // The std::string specialization exists so that generic code (float_cmp, bisect, ...) can be instantiated for
  // strings as well; its "smallest increment" is a single character.
  EXPECT_EQ("_", (Epsilon<std::string, false>::value));
  EXPECT_EQ("_", Epsilon<std::string>::value);
  EXPECT_EQ(1u, Epsilon<std::string>::value.size());
  // The integral and floating point specializations, for contrast.
  EXPECT_EQ(1, Epsilon<int>::value);
  EXPECT_DOUBLE_EQ(std::numeric_limits<double>::epsilon(), Epsilon<double>::value);
}

GTEST_TEST(OtherMath, AbsoluteDifference)
{
  EXPECT_EQ(1, absolute_difference(2, 1));
  EXPECT_EQ(1, absolute_difference(1, 2));
  EXPECT_EQ(0u, absolute_difference(3u, 3u));
  EXPECT_DOUBLE_EQ(1.5, absolute_difference(0.5, 2.0));
  // The char specialization casts back to char instead of returning the promoted int.
  EXPECT_EQ(char(2), absolute_difference(char(1), char(3)));
  EXPECT_EQ(char(2), absolute_difference(char(3), char(1)));
}

GTEST_TEST(OtherMath, FloatCmp)
{
  std::vector<double> ones{1., 1.};
  std::vector<double> twos{2., 2.};
  Dune::FieldVector<double, 2> dones(1.);
  EXPECT_TRUE(FloatCmp::eq(ones, ones));
  EXPECT_TRUE(FloatCmp::eq(dones, dones));
  EXPECT_FALSE(FloatCmp::ne(ones, ones));
  EXPECT_FALSE(FloatCmp::ne(dones, dones));

  Dune::FieldVector<double, 2> other(1.);
  other[1] = 0;
  EXPECT_TRUE(FloatCmp::ne(dones, other));
  other = 2;
  EXPECT_TRUE(FloatCmp::lt(dones, other));
  EXPECT_TRUE(FloatCmp::lt(ones, twos));
  EXPECT_TRUE(FloatCmp::gt(twos, ones));
}
