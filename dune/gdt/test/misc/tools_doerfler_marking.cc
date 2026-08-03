// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-gdt developers

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <set>

#include <dune/xt/la/container/common.hh>

#include <dune/gdt/tools/doerfler-marking.hh>

using namespace Dune;
using namespace Dune::GDT;

using V = XT::LA::CommonDenseVector<double>;


// The algorithm in doerfler_marking() squares each entry via std::pow(x, 2), sorts these squared values ascending,
// accumulates them cumulatively, and compares the cumulative sums against fractions of
// total = l2_norm(input)^2 = sum_i input_i^2.
//
// For the input {1, 2, 3, 4}:
//   squared values : 1, 4, 9, 16                 (already ascending, so sorted indices are 0, 1, 2, 3)
//   cumulative sums : 1, 5, 14, 30
//   total           : 1 + 4 + 9 + 16 = 30        (== last cumulative sum)
//
// refine set = { indices with cumulative sum >  (1 - refine_factor) * total }   (strict >)
// coarsen set = { indices with cumulative sum <  coarsen_factor      * total }   (strict <, stops at first failure)


GTEST_TEST(doerfler_marking, refine_factor_one_marks_everything)
{
  const V indicators({1., 2., 3., 4.});
  // threshold = (1 - 1) * 30 = 0, every cumulative sum (1, 5, 14, 30) is > 0
  const auto marked = doerfler_marking(indicators, /*refine_factor=*/1.);
  const std::set<size_t>& coarsen = marked.first;
  const std::set<size_t>& refine = marked.second;
  EXPECT_EQ(refine, (std::set<size_t>{0, 1, 2, 3}));
  EXPECT_TRUE(coarsen.empty());
}


GTEST_TEST(doerfler_marking, refine_factor_zero_marks_nothing)
{
  const V indicators({1., 2., 3., 4.});
  // threshold = (1 - 0) * 30 = 30, no cumulative sum is strictly > 30 (the largest equals 30)
  const auto marked = doerfler_marking(indicators, /*refine_factor=*/0.);
  const std::set<size_t>& coarsen = marked.first;
  const std::set<size_t>& refine = marked.second;
  EXPECT_TRUE(refine.empty());
  EXPECT_TRUE(coarsen.empty());
}


GTEST_TEST(doerfler_marking, marks_largest_for_refine_and_smallest_for_coarsen)
{
  const V indicators({1., 2., 3., 4.});
  // refine threshold  = (1 - 0.5) * 30 = 15, only the cumulative sum 30 (sorted index 3 -> original index 3) is > 15
  // coarsen threshold = 0.2 * 30 = 6, cumulative sums 1 and 5 are < 6 (original indices 0 and 1), stops at 14
  const auto marked = doerfler_marking(indicators, /*refine_factor=*/0.5, /*coarsen_factor=*/0.2);
  const std::set<size_t>& coarsen = marked.first;
  const std::set<size_t>& refine = marked.second;
  EXPECT_EQ(refine, (std::set<size_t>{3}));
  EXPECT_EQ(coarsen, (std::set<size_t>{0, 1}));
}
