// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers dune/xt/common/cblas.hh. Every wrapper there has two bodies: the actual CBLAS call (only compiled with
// HAVE_MKL, see the comment in cblas.cc on why only MKL's CBLAS is used) and a DUNE_THROW(dependency_missing).
// Which of the two is compiled is a property of the build, so each test below asserts the correct behaviour for
// whichever branch is active: with a backend the wrappers have to produce the documented BLAS results, without one
// they all have to throw dependency_missing (as advertised by available() returning false).

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <complex>
#include <vector>

#include <dune/xt/common/cblas.hh>
#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/float_cmp.hh>

using namespace Dune::XT::Common;

using Complex = std::complex<double>;

//! Shorthand: without a CBLAS backend every wrapper but available() throws this.
#define EXPECT_THROWS_WITHOUT_CBLAS(expression) EXPECT_THROW(expression, Exceptions::dependency_missing)


GTEST_TEST(cblas, available_matches_the_build_configuration)
{
#if HAVE_MKL
  EXPECT_TRUE(Cblas::available());
#else
  EXPECT_FALSE(Cblas::available());
#endif
  EXPECT_EQ(Cblas::available(), Cblas::available());
}


GTEST_TEST(cblas, the_constants_are_pairwise_distinct_or_throw)
{
  if (!Cblas::available()) {
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::row_major());
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::col_major());
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::left());
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::right());
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::upper());
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::lower());
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::trans());
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::no_trans());
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::unit());
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::non_unit());
    return;
  }
  // The actual numerical values are CBLAS' business, but the members of each enum have to be distinguishable.
  EXPECT_NE(Cblas::row_major(), Cblas::col_major());
  EXPECT_NE(Cblas::left(), Cblas::right());
  EXPECT_NE(Cblas::upper(), Cblas::lower());
  EXPECT_NE(Cblas::trans(), Cblas::no_trans());
  EXPECT_NE(Cblas::unit(), Cblas::non_unit());
}


GTEST_TEST(cblas, dgemv)
{
  // A = [[1, 2], [3, 4]] in row major, x = [1, 1], y = [1, 1]: y <- 2*A*x + 3*y = [9, 17].
  std::vector<double> a{1., 2., 3., 4.};
  const std::vector<double> x{1., 1.};
  std::vector<double> y{1., 1.};
  if (!Cblas::available()) {
    EXPECT_THROWS_WITHOUT_CBLAS(
        Cblas::dgemv(0, 0, 2, 2, 2., a.data(), 2, x.data(), 1, 3., y.data(), 1)); // NOLINT(bugprone-*)
    return;
  }
  Cblas::dgemv(Cblas::row_major(), Cblas::no_trans(), 2, 2, 2., a.data(), 2, x.data(), 1, 3., y.data(), 1);
  EXPECT_TRUE(FloatCmp::eq(y[0], 9.)) << "y[0] = " << y[0];
  EXPECT_TRUE(FloatCmp::eq(y[1], 17.)) << "y[1] = " << y[1];
}


GTEST_TEST(cblas, dtrsm)
{
  // Solve L * X = B with the lower triangular L = [[2, 0], [1, 2]] and B = [[2], [3]], i.e. X = [[1], [1]].
  std::vector<double> l{2., 0., 1., 2.};
  std::vector<double> b{2., 3.};
  if (!Cblas::available()) {
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::dtrsm(0, 0, 0, 0, 0, 2, 1, 1., l.data(), 2, b.data(), 1));
    return;
  }
  Cblas::dtrsm(Cblas::row_major(),
               Cblas::left(),
               Cblas::lower(),
               Cblas::no_trans(),
               Cblas::non_unit(),
               2,
               1,
               1.,
               l.data(),
               2,
               b.data(),
               1);
  EXPECT_TRUE(FloatCmp::eq(b[0], 1.)) << "b[0] = " << b[0];
  EXPECT_TRUE(FloatCmp::eq(b[1], 1.)) << "b[1] = " << b[1];
}


GTEST_TEST(cblas, dtrsv)
{
  // Same system as above, but with the single right hand side passed as a vector.
  std::vector<double> l{2., 0., 1., 2.};
  std::vector<double> x{2., 3.};
  if (!Cblas::available()) {
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::dtrsv(0, 0, 0, 0, 2, l.data(), 2, x.data(), 1));
    return;
  }
  Cblas::dtrsv(Cblas::row_major(), Cblas::lower(), Cblas::no_trans(), Cblas::non_unit(), 2, l.data(), 2, x.data(), 1);
  EXPECT_TRUE(FloatCmp::eq(x[0], 1.)) << "x[0] = " << x[0];
  EXPECT_TRUE(FloatCmp::eq(x[1], 1.)) << "x[1] = " << x[1];
}


GTEST_TEST(cblas, ztrsm)
{
  // The complex analogue of the dtrsm test: L = [[2, 0], [1, 2]], B = [[2], [3]], X = [[1], [1]].
  std::vector<Complex> l{{2., 0.}, {0., 0.}, {1., 0.}, {2., 0.}};
  std::vector<Complex> b{{2., 0.}, {3., 0.}};
  const Complex alpha{1., 0.};
  if (!Cblas::available()) {
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::ztrsm(0, 0, 0, 0, 0, 2, 1, &alpha, l.data(), 2, b.data(), 1));
    return;
  }
  Cblas::ztrsm(Cblas::row_major(),
               Cblas::left(),
               Cblas::lower(),
               Cblas::no_trans(),
               Cblas::non_unit(),
               2,
               1,
               &alpha,
               l.data(),
               2,
               b.data(),
               1);
  EXPECT_TRUE(FloatCmp::eq(b[0].real(), 1.)) << "b[0] = " << b[0];
  EXPECT_TRUE(FloatCmp::eq(b[1].real(), 1.)) << "b[1] = " << b[1];
}


GTEST_TEST(cblas, ztrsv)
{
  std::vector<Complex> l{{2., 0.}, {0., 0.}, {1., 0.}, {2., 0.}};
  std::vector<Complex> x{{2., 0.}, {3., 0.}};
  if (!Cblas::available()) {
    EXPECT_THROWS_WITHOUT_CBLAS(Cblas::ztrsv(0, 0, 0, 0, 2, l.data(), 2, x.data(), 1));
    return;
  }
  Cblas::ztrsv(Cblas::row_major(), Cblas::lower(), Cblas::no_trans(), Cblas::non_unit(), 2, l.data(), 2, x.data(), 1);
  EXPECT_TRUE(FloatCmp::eq(x[0].real(), 1.)) << "x[0] = " << x[0];
  EXPECT_TRUE(FloatCmp::eq(x[1].real(), 1.)) << "x[1] = " << x[1];
}
