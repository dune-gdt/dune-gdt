// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers dune/xt/common/mkl.hh. Both branches of every wrapper are compiled depending on HAVE_MKL, so the tests below
// only assert the behaviour which is guaranteed for either: available() has to agree with HAVE_MKL and exp() has to
// compute the element-wise exponential no matter whether it is backed by MKL's vdExp or by the std::exp fallback.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <cmath>
#include <limits>
#include <vector>

#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/mkl.hh>

using namespace Dune::XT::Common;


GTEST_TEST(mkl, available_matches_the_build_configuration)
{
#if HAVE_MKL
  EXPECT_TRUE(Mkl::available());
#else
  EXPECT_FALSE(Mkl::available());
#endif
  // available() must not change its mind between calls.
  EXPECT_EQ(Mkl::available(), Mkl::available());
}


GTEST_TEST(mkl, exp_computes_the_elementwise_exponential)
{
  const std::vector<double> input{-2., -0.5, 0., 0.5, 1., 3.};
  std::vector<double> output(input.size(), std::numeric_limits<double>::quiet_NaN());

  Mkl::exp(static_cast<int>(input.size()), input.data(), output.data());

  for (size_t ii = 0; ii < input.size(); ++ii)
    EXPECT_TRUE(FloatCmp::eq(output[ii], std::exp(input[ii])))
        << "ii = " << ii << ", output[ii] = " << output[ii] << ", std::exp(input[ii]) = " << std::exp(input[ii]);
}


GTEST_TEST(mkl, exp_may_operate_in_place)
{
  std::vector<double> values{0., 1., 2.};
  const std::vector<double> expected{std::exp(0.), std::exp(1.), std::exp(2.)};

  Mkl::exp(static_cast<int>(values.size()), values.data(), values.data());

  for (size_t ii = 0; ii < values.size(); ++ii)
    EXPECT_TRUE(FloatCmp::eq(values[ii], expected[ii])) << "ii = " << ii;
}


GTEST_TEST(mkl, exp_of_nothing_leaves_the_target_untouched)
{
  const std::vector<double> input{1., 2.};
  std::vector<double> output{42., 42.};

  Mkl::exp(0, input.data(), output.data());

  EXPECT_TRUE(FloatCmp::eq(output[0], 42.));
  EXPECT_TRUE(FloatCmp::eq(output[1], 42.));
}
