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

#include <dune/xt/common/math.hh>

#include <dune/gdt/operators/reconstruction/slopes.hh>

using namespace Dune;
using namespace Dune::GDT;

// The linear-reconstruction slope limiters in operators/reconstruction/slopes.hh reduce, per component, to the
// scalar minmod limiter from dune/xt/common/math.hh:
//   MinmodSlope::minmod(a, b): ret[ii] = XT::Common::minmod(a[ii], b[ii]);   (slopes.hh)
// We therefore verify the hand-computed limiter math on the scalar primitive that drives the reconstruction. The
// full LinearReconstructionOperator is coupled to the hyperbolic FV machinery (AnalyticalFlux, BoundaryValue and an
// EigenvectorWrapper) and cannot be instantiated in isolation, so it is intentionally not exercised here.


GTEST_TEST(operators_reconstruction, minmod_limiter_clamps_at_extrema)
{
  using XT::Common::minmod;

  // opposite signs (a local extremum) -> the limited slope is clamped to zero
  EXPECT_DOUBLE_EQ(0., minmod(2., -3.));
  EXPECT_DOUBLE_EQ(0., minmod(-2., 3.));
  EXPECT_DOUBLE_EQ(0., minmod(-1., 5.));

  // same sign -> the one smaller in magnitude
  EXPECT_DOUBLE_EQ(2., minmod(2., 3.));
  EXPECT_DOUBLE_EQ(2., minmod(3., 2.));
  EXPECT_DOUBLE_EQ(-2., minmod(-2., -3.));
  EXPECT_DOUBLE_EQ(-2., minmod(-3., -2.));

  // a zero argument yields a zero (bounded) slope
  EXPECT_DOUBLE_EQ(0., minmod(0., 5.));
  EXPECT_DOUBLE_EQ(0., minmod(5., 0.));
}


GTEST_TEST(operators_reconstruction, maxmod_limiter)
{
  using XT::Common::maxmod;

  // opposite signs -> zero
  EXPECT_DOUBLE_EQ(0., maxmod(2., -3.));
  EXPECT_DOUBLE_EQ(0., maxmod(-2., 3.));

  // same sign -> the one larger in magnitude
  EXPECT_DOUBLE_EQ(3., maxmod(2., 3.));
  EXPECT_DOUBLE_EQ(3., maxmod(3., 2.));
  EXPECT_DOUBLE_EQ(-3., maxmod(-2., -3.));
}
