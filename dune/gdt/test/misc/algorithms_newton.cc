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

#include <dune/xt/common/configuration.hh>

#include <dune/gdt/algorithms/newton.hh>

using namespace Dune;
using namespace Dune::GDT;


// default_newton_solve_options() has to provide the documented precision / iteration keys with their default values.
GTEST_TEST(algorithms_newton, default_options_have_documented_keys)
{
  const auto opts = default_newton_solve_options();
  ASSERT_TRUE(opts.has_key("precision"));
  ASSERT_TRUE(opts.has_key("max_iter"));
  ASSERT_TRUE(opts.has_key("max_dampening_iter"));
  EXPECT_DOUBLE_EQ(1e-7, opts.get<double>("precision"));
  EXPECT_EQ(100u, opts.get<size_t>("max_iter"));
  EXPECT_EQ(1000u, opts.get<size_t>("max_dampening_iter"));
}
